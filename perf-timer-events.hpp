#include "perf-timer.hpp"

std::vector<PerfEvent> get_all_events();

const PerfEvent BR_MISP_RETIRED_ALL_BRANCHES   = PerfEvent( "br_misp_retired.all_branches", "cpu/event=0xc5,umask=0x0/" );
const PerfEvent BR_MISP_RETIRED_ALL_BRANCHES_PEBS = PerfEvent( "br_misp_retired.all_branches_pebs", "cpu/event=0xc5,umask=0x4/p" );
const PerfEvent BR_MISP_RETIRED_CONDITIONAL    = PerfEvent( "br_misp_retired.conditional", "cpu/event=0xc5,umask=0x1/" );
const PerfEvent BR_MISP_RETIRED_NEAR_CALL      = PerfEvent( "br_misp_retired.near_call", "cpu/event=0xc5,umask=0x2/" );
const PerfEvent BR_MISP_RETIRED_NEAR_TAKEN     = PerfEvent( "br_misp_retired.near_taken", "cpu/event=0xc5,umask=0x20/" );
const PerfEvent CPU_CLK_UNHALTED_THREAD        = PerfEvent( "cpu_clk_unhalted.thread", "cpu/event=0x3c,umask=0x0/" );
const PerfEvent CPU_CLK_UNHALTED_THREAD_ANY    = PerfEvent( "cpu_clk_unhalted.thread_any", "cpu/event=0x3c,umask=0x0,any=1/" );
const PerfEvent CPU_CLK_UNHALTED_THREAD_P      = PerfEvent( "cpu_clk_unhalted.thread_p", "cpu/event=0x3c,umask=0x0/" );
const PerfEvent CPU_CLK_UNHALTED_THREAD_P_ANY  = PerfEvent( "cpu_clk_unhalted.thread_p_any", "cpu/event=0x3c,umask=0x0,any=1/" );
const PerfEvent CYCLE_ACTIVITY_CYCLES_L1D_MISS = PerfEvent( "cycle_activity.cycles_l1d_miss", "cpu/event=0xa3,umask=0x8,cmask=8/" );
const PerfEvent CYCLE_ACTIVITY_STALLS_L1D_MISS = PerfEvent( "cycle_activity.stalls_l1d_miss", "cpu/event=0xa3,umask=0xc,cmask=12/" );
const PerfEvent HW_INTERRUPTS_RECEIVED         = PerfEvent( "hw_interrupts.received", "cpu/event=0xcb,umask=0x1/" );
const PerfEvent INST_RETIRED_ANY               = PerfEvent( "inst_retired.any", "cpu/event=0xc0,umask=0x0/" );
const PerfEvent INST_RETIRED_ANY_P             = PerfEvent( "inst_retired.any_p", "cpu/event=0xc0,umask=0x0/" );
const PerfEvent L1D_REPLACEMENT                = PerfEvent( "l1d.replacement", "cpu/event=0x51,umask=0x1/" );
const PerfEvent L1D_PEND_MISS_FB_FULL          = PerfEvent( "l1d_pend_miss.fb_full", "cpu/event=0x48,umask=0x2/" );
const PerfEvent L1D_PEND_MISS_PENDING          = PerfEvent( "l1d_pend_miss.pending", "cpu/event=0x48,umask=0x1/" );
const PerfEvent L1D_PEND_MISS_PENDING_CYCLES   = PerfEvent( "l1d_pend_miss.pending_cycles", "cpu/event=0x48,umask=0x1,cmask=1/" );
const PerfEvent L1D_PEND_MISS_PENDING_CYCLES_ANY = PerfEvent( "l1d_pend_miss.pending_cycles_any", "cpu/event=0x48,umask=0x1,any=1,cmask=1/" );
const PerfEvent L2_RQSTS_ALL_CODE_RD           = PerfEvent( "l2_rqsts.all_code_rd", "cpu/event=0x24,umask=0xe4/" );
const PerfEvent L2_RQSTS_ALL_DEMAND_DATA_RD    = PerfEvent( "l2_rqsts.all_demand_data_rd", "cpu/event=0x24,umask=0xe1/" );
const PerfEvent L2_RQSTS_ALL_DEMAND_MISS       = PerfEvent( "l2_rqsts.all_demand_miss", "cpu/event=0x24,umask=0x27/" );
const PerfEvent L2_RQSTS_ALL_DEMAND_REFERENCES = PerfEvent( "l2_rqsts.all_demand_references", "cpu/event=0x24,umask=0xe7/" );
const PerfEvent L2_RQSTS_ALL_PF                = PerfEvent( "l2_rqsts.all_pf", "cpu/event=0x24,umask=0xf8/" );
const PerfEvent L2_RQSTS_ALL_RFO               = PerfEvent( "l2_rqsts.all_rfo", "cpu/event=0x24,umask=0xe2/" );
const PerfEvent L2_RQSTS_CODE_RD_HIT           = PerfEvent( "l2_rqsts.code_rd_hit", "cpu/event=0x24,umask=0xc4/" );
const PerfEvent L2_RQSTS_CODE_RD_MISS          = PerfEvent( "l2_rqsts.code_rd_miss", "cpu/event=0x24,umask=0x24/" );
const PerfEvent L2_RQSTS_DEMAND_DATA_RD_HIT    = PerfEvent( "l2_rqsts.demand_data_rd_hit", "cpu/event=0x24,umask=0xc1/" );
const PerfEvent L2_RQSTS_DEMAND_DATA_RD_MISS   = PerfEvent( "l2_rqsts.demand_data_rd_miss", "cpu/event=0x24,umask=0x21/" );
const PerfEvent L2_RQSTS_MISS                  = PerfEvent( "l2_rqsts.miss", "cpu/event=0x24,umask=0x3f/" );
const PerfEvent L2_RQSTS_PF_HIT                = PerfEvent( "l2_rqsts.pf_hit", "cpu/event=0x24,umask=0xd8/" );
const PerfEvent L2_RQSTS_PF_MISS               = PerfEvent( "l2_rqsts.pf_miss", "cpu/event=0x24,umask=0x38/" );
const PerfEvent L2_RQSTS_REFERENCES            = PerfEvent( "l2_rqsts.references", "cpu/event=0x24,umask=0xff/" );
const PerfEvent L2_RQSTS_RFO_HIT               = PerfEvent( "l2_rqsts.rfo_hit", "cpu/event=0x24,umask=0xc2/" );
const PerfEvent L2_RQSTS_RFO_MISS              = PerfEvent( "l2_rqsts.rfo_miss", "cpu/event=0x24,umask=0x22/" );
const PerfEvent MEM_INST_RETIRED_ALL_LOADS     = PerfEvent( "mem_inst_retired.all_loads", "cpu/event=0xd0,umask=0x81/" );
const PerfEvent MEM_INST_RETIRED_ALL_STORES    = PerfEvent( "mem_inst_retired.all_stores", "cpu/event=0xd0,umask=0x82/" );
const PerfEvent MEM_INST_RETIRED_LOCK_LOADS    = PerfEvent( "mem_inst_retired.lock_loads", "cpu/event=0xd0,umask=0x21/" );
const PerfEvent MEM_INST_RETIRED_SPLIT_LOADS   = PerfEvent( "mem_inst_retired.split_loads", "cpu/event=0xd0,umask=0x41/" );
const PerfEvent MEM_INST_RETIRED_SPLIT_STORES  = PerfEvent( "mem_inst_retired.split_stores", "cpu/event=0xd0,umask=0x42/" );
const PerfEvent MEM_INST_RETIRED_STLB_MISS_LOADS = PerfEvent( "mem_inst_retired.stlb_miss_loads", "cpu/event=0xd0,umask=0x11/" );
const PerfEvent MEM_INST_RETIRED_STLB_MISS_STORES = PerfEvent( "mem_inst_retired.stlb_miss_stores", "cpu/event=0xd0,umask=0x12/" );
const PerfEvent MEM_LOAD_RETIRED_FB_HIT        = PerfEvent( "mem_load_retired.fb_hit", "cpu/event=0xd1,umask=0x40/" );
const PerfEvent MEM_LOAD_RETIRED_L1_HIT        = PerfEvent( "mem_load_retired.l1_hit", "cpu/event=0xd1,umask=0x1/" );
const PerfEvent MEM_LOAD_RETIRED_L1_MISS       = PerfEvent( "mem_load_retired.l1_miss", "cpu/event=0xd1,umask=0x8/" );
const PerfEvent MEM_LOAD_RETIRED_L2_HIT        = PerfEvent( "mem_load_retired.l2_hit", "cpu/event=0xd1,umask=0x2/" );
const PerfEvent MEM_LOAD_RETIRED_L2_MISS       = PerfEvent( "mem_load_retired.l2_miss", "cpu/event=0xd1,umask=0x10/" );
const PerfEvent MEM_LOAD_RETIRED_L3_HIT        = PerfEvent( "mem_load_retired.l3_hit", "cpu/event=0xd1,umask=0x4/" );
const PerfEvent MEM_LOAD_RETIRED_L3_MISS       = PerfEvent( "mem_load_retired.l3_miss", "cpu/event=0xd1,umask=0x20/" );
const PerfEvent UOPS_DISPATCHED_PORT_PORT_0    = PerfEvent( "uops_dispatched_port.port_0", "cpu/event=0xa1,umask=0x1/" );
const PerfEvent UOPS_DISPATCHED_PORT_PORT_1    = PerfEvent( "uops_dispatched_port.port_1", "cpu/event=0xa1,umask=0x2/" );
const PerfEvent UOPS_DISPATCHED_PORT_PORT_2    = PerfEvent( "uops_dispatched_port.port_2", "cpu/event=0xa1,umask=0x4/" );
const PerfEvent UOPS_DISPATCHED_PORT_PORT_3    = PerfEvent( "uops_dispatched_port.port_3", "cpu/event=0xa1,umask=0x8/" );
const PerfEvent UOPS_DISPATCHED_PORT_PORT_4    = PerfEvent( "uops_dispatched_port.port_4", "cpu/event=0xa1,umask=0x10/" );
const PerfEvent UOPS_DISPATCHED_PORT_PORT_5    = PerfEvent( "uops_dispatched_port.port_5", "cpu/event=0xa1,umask=0x20/" );
const PerfEvent UOPS_DISPATCHED_PORT_PORT_6    = PerfEvent( "uops_dispatched_port.port_6", "cpu/event=0xa1,umask=0x40/" );
const PerfEvent UOPS_DISPATCHED_PORT_PORT_7    = PerfEvent( "uops_dispatched_port.port_7", "cpu/event=0xa1,umask=0x80/" );
const PerfEvent UOPS_ISSUED_ANY                = PerfEvent( "uops_issued.any", "cpu/event=0xe,umask=0x1/" );
const PerfEvent NoEvent = {"",""};