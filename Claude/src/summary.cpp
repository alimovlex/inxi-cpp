#include "summary.h"
#include "common.h"
#include "cpu_module.h"
#include "memory_module.h"
#include "storage_module.h"
#include "info_module.h"
#include "system_module.h"
#include "version.h"

namespace mod {
using namespace util;

void printDefaultSummary(std::ostream& os, bool color) {
    SectionPrinter p(os, color);

    CpuSummary cpu = getCpuSummary();
    MemInfo mem = getMemInfo();
    double driveTotal = getTotalDriveBytes();
    double driveUsed = getTotalUsedPartitionBytes();

    p.beginLine(0);
    p.field("CPU", cpu.model + " (" + std::to_string(cpu.physicalCores) + "-core)");
    if (cpu.currentMHz > 0) {
        p.field("speed", std::to_string(static_cast<long>(cpu.currentMHz)) + " MHz");
    }
    p.field("Kernel", getKernelRelease() + " " + getKernelArch());
    p.field("Up", formatUptime(getUptimeSeconds()));
    if (mem.totalBytes > 0) {
        p.field("Mem", humanSize(mem.usedBytes) + "/" + humanSize(mem.totalBytes) +
                        " (" + percentStr(mem.usedBytes, mem.totalBytes) + ")");
    }
    if (driveTotal > 0) {
        p.field("Storage", humanSize(driveTotal) + " (" + percentStr(driveUsed, driveTotal) + " used)");
    }
    p.field("Procs", std::to_string(getProcessCount()));
    p.field("Shell", getShellName());
    p.field(PROGRAM_NAME, PROGRAM_VERSION);
    p.endLine();
}

} // namespace mod
