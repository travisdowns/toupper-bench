
#include <assert.h>
#include "common-cxx.hpp"
#include "cycle-timer.h"
#include "env.hpp"
#include "huge-alloc.h"
#include "impl-list.hpp"
#include "misc.hpp"
// #include "nlohmann/json.hpp"
#include "opt-control.h"
#include "pcg-cpp/pcg_random.hpp"
#include "perf-timer-events.hpp"
#include "perf-timer.hpp"
#include "randutil.hpp"
#include "string-instrument.hpp"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

#include <math.h>
#include <sys/mman.h>
#include <time.h>

#include <emmintrin.h>
#include <immintrin.h>

#include <sched.h>

using namespace env;

static bool verbose;
static bool summary;
static bool debug;
static bool do_json;

using velem = std::vector<char>;

#define vprint(...)                       \
    do {                                  \
        if (verbose)                      \
            fprintf(stderr, __VA_ARGS__); \
    } while (false)
#define dprint(...)                       \
    do {                                  \
        if (debug)                        \
            fprintf(stderr, __VA_ARGS__); \
    } while (false)

/**
 * Allocate size elems, using huge_alloc which reports on THP stuff.
 *
 * The optional offset specifies the offset relative to a 2 MiB page,
 * helpful to ensure different pointers have a specific relative
 * alignment.
 */
char* alloc(size_t size, size_t offset = 0) {
    size_t fullsize = size + offset;
    char* p = (char*)huge_alloc(fullsize * sizeof(char), false);  // !summary);
    return p + offset;
}

/**
 * Allocate size elements and fill them with the sequence 0, 1, 2 ...
 */
HEDLEY_NEVER_INLINE
char_span alloc_random(size_t size, size_t offset = 0) {
    std::mt19937_64 rng;
    std::uniform_int_distribution<char> dist(32, 127);
    char* a = alloc(size, offset);
    std::generate(a, a + size, [&](){ return dist(rng); });
    return {a, size};
}

void pinToCpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set)) {
        assert("pinning failed" && false);
    }
}

template <typename... Args>
void usageCheck(bool condition, const std::string& fmt, Args... args) {
    if (!condition) {
        fprintf(stderr,
                "%s\n"
                "Usage:\n"
                "\tbench [TEST_NAME]\n"
                "\n where you can optionally provide TEST_NAME to run a specific test:\n\n",
                string_format(fmt, args...).c_str());

        for (auto& desc : get_all()) {
            fprintf(stderr, " %s\n\t%s\n", desc.name, desc.desc);
        }
        exit(EXIT_FAILURE);
    }
}

/* dump the tests in a single space-separate strings, perhaps convenient so you can do something like:
 *     for test in $(DUMPTESTS=1 ./bench); do ./bench $test; done
 * to run all tests. */
void dump_tests() {
    for (auto& desc : get_all()) {
        printf("%s ", desc.name);
    }
}

/* return the alignment of the given pointer
   i.e., the largest power of two that divides it */
size_t get_alignment(const void* p) {
    return (size_t)((1UL << __builtin_ctzl((uintptr_t)p)));
}

/**
 * Get the relative alignment of two pointers. The relative alignment is determined
 * by the number of equal least significant bits.
 */
size_t relalign(const void* a, const void* b) {
    return get_alignment((void*)((uintptr_t)a ^ (uintptr_t)b));
}

void update_min_counts(bool first, event_counts* min, event_counts cur) {
    for (size_t i = 0; i < MAX_COUNTERS; i++) {
        min->counts[i] = (first || cur.counts[i] < min->counts[i] ? cur.counts[i] : min->counts[i]);
    }
}

struct iprinter;

struct BenchArgs {
    char_span input;
    char* output;
    size_t repeat_count, iters;
    iprinter* printer;

    /**
     * Get the intersect args appropriate for the given iteration.
     */
    bench_args get_args() const {
        std::copy(input.begin(), input.end(), output);
        return {output, input.size()};
    }
};

class StampConfig;

/**
 * Test stamp encapsulates everything we measure before and after the code
 * being benchmarked.
 */
class Stamp {
    friend StampConfig;

    cl_timepoint cycle_stamp;
    event_counts counters;
    string_metrics::Map string_map;

    Stamp(cl_timepoint cycle_stamp, event_counts counters, string_metrics::Map string_map)
        : cycle_stamp{cycle_stamp}, counters{counters}, string_map{std::move(string_map)} {}

public:
    std::string to_string() { return std::string("cs: ") + std::to_string(this->cycle_stamp.nanos); }
};

class StampConfig;

/**
 * Thrown when the caller asks for a counter that was never configured.
 */
struct NonExistentCounter : public std::logic_error {
    NonExistentCounter(const PerfEvent& e) : std::logic_error(std::string("counter ") + e.name + " doesn't exist") {}
};

/**
 * Taking the delta of two stamps gives you a stamp delta.
 *
 * The StampData has a reference to the StampConfig from which it was created, so the
 * lifetime of the StampConfig must be at least as long as the StampDelta.
 */
class StampDelta {
    friend Stamp;
    friend StampConfig;

    bool empty;
    const StampConfig* config;
    // not cycles: has arbitrary units
    cl_interval cycle_delta;
    event_counts counters;
    string_metrics::Map string_map;

    StampDelta(const StampConfig& config,
               cl_interval cycle_delta,
               event_counts counters,
               string_metrics::Map string_map)
        : empty(false),
          config{&config},
          cycle_delta{cycle_delta},
          counters{std::move(counters)},
          string_map{std::move(string_map)} {}

public:
    /**
     * Create an "empty" delta - the only thing an empty delta does is
     * never be returned from functions like min(), unless both arguments
     * are empty. Handy for accumulation patterns.
     */
    StampDelta() : empty(true), config{nullptr}, cycle_delta{}, counters{}, string_map{} {}

    double get_nanos() const {
        assert(!empty);
        return cl_to_nanos(cycle_delta);
    }

    double get_cycles() const {
        assert(!empty);
        return cl_to_cycles(cycle_delta);
    }

    event_counts get_counters() const {
        assert(!empty);
        return counters;
    }

    uint64_t get_counter(const PerfEvent& event) const;

    const string_metrics::Map& get_string_metrics() const { return string_map; }

    std::string to_string() { return std::string("cd: ") + std::to_string(cycle_delta.nanos); }

    /**
     * Return a new StampDelta with every contained element having the minimum
     * value between the left and right arguments.
     *
     * As a special rule, if either argument is empty, the other argument is returned
     * without applying the function, this facilitates typical use with an initial
     * empty object followed by accumulation.
     */
    template <typename F>
    static StampDelta apply(const StampDelta& l, const StampDelta& r, F f) {
        if (l.empty)
            return r;
        if (r.empty)
            return l;
        assert(l.config == r.config);
        event_counts new_counts            = event_counts::apply(l.counters, r.counters, f);
        string_metrics::Map new_string_map = string_metrics::Map::apply(l.string_map, r.string_map, f);
        return StampDelta{*l.config, {f(l.cycle_delta.nanos, r.cycle_delta.nanos)}, new_counts, new_string_map};
    }

    static StampDelta min(const StampDelta& l, const StampDelta& r) { return apply(l, r, min_functor{}); }
    static StampDelta max(const StampDelta& l, const StampDelta& r) { return apply(l, r, max_functor{}); }
};

const PerfEvent DUMMY_EVENT_US = PerfEvent("microseconds", "microsecond");

/**
 * Manages PMU events.
 */
class EventManager {
    /** event to counter index */
    std::map<PerfEvent, size_t> event_map;
    std::vector<PerfEvent> event_vec;
    std::vector<bool> setup_results;
    size_t next_counter;
    bool prepared;

public:
    EventManager() : next_counter(0), prepared(false) {}

    bool add_event(const PerfEvent& event) {
        if (event == NoEvent || event == DUMMY_EVENT_US) {
            return true;
        }
        vprint("Adding event %s\n", to_string(event).c_str());
        prepared = false;
        if (event_map.count(event)) {
            return true;
        }
        if (event_map.size() == MAX_COUNTERS) {
            return false;
        }
        event_map.insert({event, next_counter++});
        event_vec.push_back(event);
        return true;
    }

    void prepare() {
        assert(event_map.size() == event_vec.size());
        setup_results   = setup_counters(event_vec);
        size_t failures = std::count(setup_results.begin(), setup_results.end(), false);
        if (failures > 0) {
            fprintf(stderr, "%zu events failed to be configured\n", failures);
        }
        vprint("EventManager configured %zu events\n", (setup_results.size() - failures));
        prepared = true;
    }

    /**
     * Return the counter slot for the given event, or -1
     * if the event was requested but setting up the counter
     * failed.
     *
     * If an event is requested that was never configured,
     * NonExistentCounter exception is thrown.
     */
    ssize_t get_mapping(const PerfEvent& event) const {
        if (!prepared) {
            throw std::logic_error("not prepared");
        }
        auto mapit = event_map.find(event);
        if (mapit == event_map.end()) {
            throw NonExistentCounter(event);
        }
        size_t idx = mapit->second;
        if (!setup_results.at(idx)) {
            return -1;
        }
        return idx;
    }
};

/**
 * A class that holds configuration for creating stamps.
 *
 * Configured based on what columns are requested, holds configuration for varous types
 * of objects.
 */
class StampConfig {
public:
    EventManager em;
    bool do_string_intrument = true;

    /**
     * After updating the config to the state you want, call prepare() once which
     * does any global configuration needed to support the configured stamps, such
     * as programming PMU events.
     */
    void prepare() { em.prepare(); }

    // take the stamp.
    Stamp stamp() const {
        auto cycle_stamp = cl_now();
        auto counters    = read_counters();
        string_metrics::Map string_map;
        if (do_string_intrument) {
            string_map = string_metrics::global();
        }
        return Stamp(cycle_stamp, counters, string_map);
    }

    /**
     * Create a StampDelta from the given before/after stamps
     * which should have been created by this StampConfig.
     */
    StampDelta delta(const Stamp& before, const Stamp& after) const {
        return StampDelta(*this, cl_delta(before.cycle_stamp, after.cycle_stamp),
                          calc_delta(before.counters, after.counters),
                          string_metrics::Map::delta(before.string_map, after.string_map));
    }
};

uint64_t StampDelta::get_counter(const PerfEvent& event) const {
    const EventManager& em = config->em;
    ssize_t idx            = em.get_mapping(event);
    assert(idx >= -1 && idx <= MAX_COUNTERS);
    if (idx == -1) {
        return -1;
    }
    return this->counters.counts[idx];
}

struct BenchResults {
    StampDelta delta;
    BenchArgs args;

    BenchResults() = delete;

    size_t ssize() const { return args.input.size(); }
};

/** thrown by get_value if a column failed for some reason */
struct ColFailed : std::runtime_error {
    /* colval_ is used as the column value */
    std::string colval_;
    ColFailed(std::string colval) : std::runtime_error("column failed"), colval_{std::move(colval)} {}
};

/**
 * A Column object represents a thing which knows how to print a column of data.
 * It injects what it wants into the Stamp object and then gets it back out after.
 *
 *
 */
class Column {
public:
    /**
     * The normalization mode determines how the value is normalized, e.g., made
     * to reflect a value which is indepednent of the number of iterations, etc.
     */
    enum NormMode {
        /* No normalization needed, e.g., because the value is a ratio */
        NORM_NONE,
        /* Normalization for values that are derived from instruments associated
         * with run_instrumented. That is, normalize the value based on the number
         * of iterations used by run_intrumented. Normalizes per small element. */
        NORM_INSTRUMENT,
        /* Normalize based on the main test, i.e., value that come from the main
         * benchmark loop. Normalizes per small element. */
        NORM_FULL,
        /* Normalize only by the number of inner iterations, not by the number of small
           elements. */
        NORM_ITERS_ONLY

    };

private:
    const char* heading;
    const char* format;
    NormMode norm_mode;
    bool post_output;

protected:
    /* subclasses implement this to return the value associated with the metric */
    virtual std::pair<double, bool> get_value(const BenchResults& results) const {
        throw std::logic_error("unimplemented get_value");
    };

public:
    Column(const char* heading, const char* format, NormMode norm_mode, bool post_output = false)
        : heading{heading}, format{format}, norm_mode{norm_mode}, post_output{post_output} {}

    virtual ~Column() {}

    virtual const char* get_header() const { return heading; }
    virtual int col_width() const { return std::max(4, (int)strlen(heading)) + 1; }

    /* subclasses can implement this to modify the StampConfig as needed in order to get the values needed
       for this column */
    virtual void update_config(StampConfig& sc) const {}

    /**
     * "post output" columns don't get get printed in the table like
     * other columns but rather are output one at a time after each
     * repeat, since they output a lot of data
     */
    virtual bool is_post_output() const { return post_output; }

    double get_final_value(const BenchResults& results) const {
        auto val = get_value(results);
        if (val.second) {
            return val.first / get_norm_divisor(results);
        } else {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    virtual void print(FILE* f, const BenchResults& results) const { print(f, get_final_value(results)); }

    void print(FILE* f, double val) const { print(f, formatted_string(val)); }

    void print(FILE* f, const std::string& s) const {
        auto formatted = string_format("%*s", col_width(), s.c_str());
        if ((int)formatted.size() > col_width())
            formatted.resize(col_width());
        fprintf(f, "%s", formatted.c_str());
    }

    /** return the string representation of the results */
    virtual std::string formatted_string(double val) const {
        try {
            if (std::isnan(val)) {
                return string_format("%*s", col_width(), "-");
            } else {
                return string_format(format, col_width(), val);
            }
        } catch (ColFailed& fail) {
            auto ret = string_format("%*s", col_width(), fail.colval_.c_str());
            if ((int)ret.size() > col_width())
                ret.resize(col_width());
            return ret;
        }
    }

    double get_norm_divisor(const BenchResults& results) const {
        switch (norm_mode) {
            case NORM_NONE:
                return 1;
            case NORM_INSTRUMENT:
                return results.ssize();  // instrument doesn't do "iters" inner loops
            case NORM_FULL:
                return results.args.iters * results.ssize();
            case NORM_ITERS_ONLY:
                return results.args.iters;
        }
        throw std::logic_error("get_norm_divisor");
    }
};

class EventColumn : public Column {
public:
    PerfEvent top, bottom;

    EventColumn(const char* heading, const char* format, PerfEvent top, PerfEvent bottom)
        : Column{heading, format, bottom == NoEvent ? NORM_FULL : NORM_NONE}, top{top}, bottom{bottom} {}

    virtual std::pair<double, bool> get_value(const BenchResults& results) const override {
        double ratio = value(results.delta, top) / (is_ratio() ? value(results.delta, bottom) : 1.);
        return {ratio, true};
    }

    void update_config(StampConfig& sc) const override {
        sc.em.add_event(top);
        sc.em.add_event(bottom);
    }

    /** true if this value is a ratio (no need to normalize), false otherwise */
    bool is_ratio() const {
        return bottom != NoEvent;  // lol
    }

private:
    double value(const StampDelta& delta, const PerfEvent& e) const {
        if (e == DUMMY_EVENT_US) {
            return delta.get_nanos() / 1000.;
        }
        auto v = delta.get_counter(e);
        if (v == (uint64_t)-1) {
            throw ColFailed("fail");
        }
        return v;
    }
};

EventColumn EVENT_COLUMNS[] = {

        {"INSTRU", "%*.2f", INST_RETIRED_ANY, NoEvent},
        {"True Cycles", "%*.2f", CPU_CLK_UNHALTED_THREAD, NoEvent},
        {"IPC", "%*.2f", INST_RETIRED_ANY, CPU_CLK_UNHALTED_THREAD},
        {"UPC", "%*.2f", UOPS_ISSUED_ANY, CPU_CLK_UNHALTED_THREAD},
        {"MLP1A", "%*.2f", L1D_PEND_MISS_PENDING, CPU_CLK_UNHALTED_THREAD},
        {"MLP1B", "%*.2f", L1D_PEND_MISS_PENDING, L1D_PEND_MISS_PENDING_CYCLES},
        {"LAT", "%*.0f", L1D_PEND_MISS_PENDING, MEM_LOAD_RETIRED_L1_MISS},
        {"LOADO %", "%*.2f", L1D_PEND_MISS_PENDING_CYCLES, CPU_CLK_UNHALTED_THREAD},
        {"ALL_LOAD", "%*.2f", MEM_INST_RETIRED_ALL_LOADS, NoEvent},
        {"L1_MISS", "%*.2f", MEM_LOAD_RETIRED_L1_MISS, NoEvent},
        {"L1_REPL", "%*.2f", L1D_REPLACEMENT, NoEvent},
        {"BR_MISP", "%*.2f", BR_MISP_RETIRED_ALL_BRANCHES, NoEvent},

        {"MHz", "%*.0f", CPU_CLK_UNHALTED_THREAD, DUMMY_EVENT_US},

        {"P0", "%*.2f", UOPS_DISPATCHED_PORT_PORT_0, NoEvent},
        {"P1", "%*.2f", UOPS_DISPATCHED_PORT_PORT_1, NoEvent},
        {"P2", "%*.2f", UOPS_DISPATCHED_PORT_PORT_2, NoEvent},
        {"P3", "%*.2f", UOPS_DISPATCHED_PORT_PORT_3, NoEvent},
        {"P4", "%*.2f", UOPS_DISPATCHED_PORT_PORT_4, NoEvent},
        {"P5", "%*.2f", UOPS_DISPATCHED_PORT_PORT_5, NoEvent},
        {"P6", "%*.2f", UOPS_DISPATCHED_PORT_PORT_6, NoEvent},
        {"P7", "%*.2f", UOPS_DISPATCHED_PORT_PORT_7, NoEvent},

};

/**
 * The simplest column just lets you specify an "extractor" function to return the value given a
 * BenchResults object.
 */
class SimpleColumn : public Column {
public:
    using extractor_fn = std::function<double(const BenchResults& ir)>;
    extractor_fn extractor;

    SimpleColumn(const char* heading, const char* format, extractor_fn extractor, NormMode norm_mode = NORM_FULL)
        : Column{heading, format, norm_mode}, extractor{extractor} {}

    virtual std::pair<double, bool> get_value(const BenchResults& results) const override {
        return {extractor(results), true};
    }
};

using BR = const BenchResults&;

SimpleColumn BASIC_COLUMNS[] = {
        {"Cycles", "%*.2f", [](BR r) { return r.delta.get_cycles(); }},
};

using ColList = std::vector<Column*>;

/**
 * Get all the available columns
 */
ColList get_all_columns() {
    ColList ret;
    auto add = [&ret](auto& container) {
        for (auto& c : container) {
            ret.push_back(&c);
        }
    };
    add(BASIC_COLUMNS);
    add(EVENT_COLUMNS);
    return ret;
}

struct iprinter {
    /** called once before any output */
    virtual void print_start() {}

    /** called once before any output */
    virtual void print_end() {}

    virtual void print_one(const test_description* test,
                           const std::vector<BenchResults>& results,
                           const ColList& columns,
                           const ColList& post_columns) = 0;
};

struct stdout_printer : iprinter {
    virtual void print_one(const test_description* test,
                           const std::vector<BenchResults>& results,
                           const ColList& columns,
                           const ColList& post_columns) override {
        printf("\n----------------------------------------\n");

        if (!summary) {
            fprintf(stdout, "Running test %s : %s\n", test->name, test->desc);
        }

        printf("  iter");
        for (auto col : columns) {
            printf(" |%*s", col->col_width(), col->get_header());
        }
        printf("\n");

        std::vector<double> minvals(columns.size(), std::numeric_limits<double>::quiet_NaN());
        std::vector<double> maxvals(columns.size(), std::numeric_limits<double>::quiet_NaN());
        // , maxvals
        for (size_t repeat = 0; repeat < results.size(); repeat++) {
            auto& result = results.at(repeat);

            printf("%6zu  ", repeat);

            for (size_t c = 0; c < columns.size(); c++) {
                auto& column = columns[c];
                // column->print(stdout, result);
                try {
                    double val = column->get_final_value(result);
                    column->print(stdout, val);
                    printf("  ");
                    if (!std::isnan(val)) {
                        minvals[c] = std::fmin(minvals[c], val);
                        maxvals[c] = std::fmax(maxvals[c], val);
                    }
                } catch (const ColFailed& failed) {
                    column->print(stdout, failed.colval_);
                }
            }
            printf("\n");

            // print any "big" post stuff
            for (auto post : post_columns) {
                post->print(stdout, result);
            }
        }

        auto printline = [&columns](const char* what, const std::vector<double>& vals) {
            printf("%6s  ", what);
            for (size_t c = 0; c < columns.size(); c++) {
                columns[c]->print(stdout, vals[c]);
                printf("  ");
            }
            printf("\n");
        };

        printline("min", minvals);
        printline("max", maxvals);

        printf("\n ----------------------------------------\n");
    }
};

struct json_printer : iprinter {
    static const size_t INDENT = 2;

    FILE* f = stdout;
    std::vector<test_description> tests;
    ColList columns;
    int indent = 0;
    /** fresh == true means no need for comma */
    bool fresh = true;

    json_printer(std::vector<test_description> tests, const ColList& columns) : tests{tests}, columns{columns} {}

    void print(const std::string& s) { fprintf(f, "%s", s.c_str()); }

    void do_indent() {
        std::string spaces(INDENT * indent, ' ');
        print(spaces);
    }

    void maybe_comma() {
        if (!fresh) {
            print(",\n");
        }
        fresh = false;
    }

    void start_array(const std::string& name) {
        maybe_comma();
        do_indent();
        fprintf(f, "\"%s\" : [", name.c_str());
    }

    void end_array() { fprintf(f, "]"); }

    void start_object(const std::string& key) {
        maybe_comma();
        do_indent();
        fprintf(f, "\"%s\" : {\n", key.c_str());
        ++indent;
        fresh = true;
    }

    void end_object() {
        --indent;
        print("\n");
        do_indent();
        print("}");
        fresh = false;
    };

    template <typename C>
    void print_array(const std::string& name, const C& container, const char* format) {
        start_array(name);
        bool first = true;
        for (auto& val : container) {
            if (!first)
                fprintf(f, ", ");
            first = false;
            fprintf(f, format, val);
        }
        end_array();
    }

    virtual void print_start() override {
        // write the top level stuff and open the results object
        print("{\n");
        ++indent;

        // column names
        std::vector<const char*> cnames;
        std::transform(columns.begin(), columns.end(), std::back_inserter(cnames),
                       [](const Column* c) { return c->get_header(); });
        print_array("columns", cnames, "\"%s\"");

        std::vector<const char*> bnames;
        std::transform(tests.begin(), tests.end(), std::back_inserter(bnames),
                       [](test_description t) { return t.name; });
        print_array("benches", bnames, "\"%s\"");

        start_object("results");
    }

    virtual void print_end() override {
        end_object();
        --indent;
        print("\n}\n");
    }

    virtual void print_one(const test_description* test,
                           const std::vector<BenchResults>& results,
                           const ColList& columns,
                           const ColList& post_columns) override {
        start_object(test->name);

        for (size_t c = 0; c < columns.size(); c++) {
            auto& column = columns[c];

            std::vector<std::string> doubles;
            std::vector<const char*> doubles_cstr;
            doubles.reserve(results.size());
            for (auto& result : results) {
                try {
                    auto d = column->get_final_value(result);
                    doubles.push_back(std::isnan(d) ? "\"NaN\"" : string_format("%.6f", d));
                    doubles_cstr.push_back(doubles.back().c_str());
                } catch (const ColFailed& failed) {
                    break;
                }
            }

            if (doubles.size() == results.size()) {
                print_array(column->get_header(), doubles_cstr, "%s");
            } else {
                // the column failed for this algorithm, do nothing for now
            }
        }

        end_object();
    }
};

void runOne(const test_description* test,
            const StampConfig& config,
            const ColList& columns,
            const ColList& post_columns,
            const BenchArgs& bargs) {
    /* the main benchmark loop */
    std::vector<BenchResults> result_array;
    result_array.reserve(bargs.repeat_count);
    for (size_t repeat = 0; repeat < bargs.repeat_count; repeat++) {
        Stamp before = config.stamp();
        for (size_t c = 0; c < bargs.iters; ++c) {
            auto args = bargs.get_args();
            test->call_f(args);
        }
        Stamp after = config.stamp();

        StampDelta delta = config.delta(before, after);

        dprint("before: %s\n after: %s\n delta: %s\n", before.to_string().c_str(), after.to_string().c_str(),
               delta.to_string().c_str());

        result_array.push_back(BenchResults{delta, bargs});
    }

    bargs.printer->print_one(test, result_array, columns, post_columns);
}

int main(int argc, char** argv) {
    summary = getenv_bool("SUMMARY");
    verbose = !getenv_bool("QUIET");
    debug   = getenv_bool("DEBUG");
    do_json = getenv_bool("JSON");

    bool dump_tests_flag = getenv_bool("DUMPTESTS");
    bool do_list_events  = getenv_bool("LIST_EVENTS");  // list the events and quit
    bool include_slow    = getenv_bool("INCLUDE_SLOW");
    std::string collist  = getenv_generic<std::string>(
            "COLS",
            "Cycles,INSTRU,IPC,UPC,LOADO %,MLP1A,MLP1B,LAT,ALL_LOAD,L1_MISS,L1_REPL");

    size_t size = getenv_int("SIZE", 2 * 1024);
    int pincpu   = getenv_int("PINCPU", 0);
    size_t iters = getenv_int("ITERS", 100);

    assert(iters > 0);

    if (verbose)
        set_verbose(true);  // set perf-timer to verbose too

    if (dump_tests_flag) {
        dump_tests();
        exit(EXIT_SUCCESS);
    }

    if (do_list_events) {
        list_events();
        exit(EXIT_SUCCESS);
    }

    usageCheck(argc == 1 || argc == 2, "Must provide 0 or 1 arguments");

    std::vector<test_description> tests;

    if (argc > 1) {
        tests = get_by_list(argv[1]);
    } else {
        // all tests
        for (auto t : get_all()) {
            if ((include_slow || !(t.flags & SLOW))) {
                tests.push_back(t);
            }
        }
    }

    pinToCpu(pincpu);

    ColList allcolumns, columns, post_columns;
    for (auto requested : split(collist, ",")) {
        if (requested.empty()) {
            continue;
        }
        bool found = false;
        for (auto& col : get_all_columns()) {
            if (requested == col->get_header()) {
                allcolumns.push_back(col);
                found = true;
                break;
            }
        }
        usageCheck(found, "No column named %s", requested.c_str());
    }

    std::copy_if(allcolumns.begin(), allcolumns.end(), std::back_inserter(columns),
                 [](auto& c) { return !c->is_post_output(); });
    std::copy_if(allcolumns.begin(), allcolumns.end(), std::back_inserter(post_columns),
                 [](auto& c) { return c->is_post_output(); });

    vprint("Found %zu normal columns and %zu post-output columns\n", columns.size(), post_columns.size());

    StampConfig config;
    // we give each column a chance to update the StampConfig with what it needs
    for (auto& col : allcolumns) {
        col->update_config(config);
    }
    config.prepare();

    cl_init(!summary);

    // run the whole test repeat_count times, each of which calls the test function iters times
    unsigned repeat_count = 10;

    char_span input = alloc_random(size, 0);
    char* output = alloc(size, 0);  // output same size as input array since there can't be more than that many matches

    if (verbose) {
        fprintf(stderr, "inner loops : %10zu\n", iters);
        fprintf(stderr, "pinned cpu  : %10d\n", pincpu);
        fprintf(stderr, "current cpu : %10d\n", sched_getcpu());
        fprintf(stderr, "input count : %10zu elems\n", size);
        fprintf(stderr, "input size  : %10zu bytes\n", size * sizeof(char));
        fprintf(stderr, "input align : %10zu\n", get_alignment(input.data()));
        fprintf(stderr, "output align: %10zu\n", get_alignment(output));
        fprintf(stderr, "rel align   : %10zu\n", relalign(input.data(), output));
    }

    if (!summary) {
        fprintf(stderr, "About to run %zu tests with %zu columns (after %zu ms of startup time)\n", tests.size(),
                columns.size(), (size_t)clock() * 1000u / CLOCKS_PER_SEC);
    }

    auto printer = do_json ? (iprinter*)new json_printer(tests, columns) : new stdout_printer;

    printer->print_start();

    for (auto t : tests) {
        runOne(&t, config, columns, post_columns, {input, output, repeat_count, iters, printer});
    }

    printer->print_end();
}
