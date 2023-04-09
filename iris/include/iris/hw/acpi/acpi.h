#pragma once

#include <iris/hw/acpi/system_tables.h>

namespace iris::acpi {
struct AcpiInformation {
    di::Optional<MADT> madt;
    di::Optional<ProcessorLocalApicStructure> local_apic;
    di::Vector<IoApicStructure> io_apics;
    di::Vector<InterruptSourceOverrideStructure> interrupt_source_overrides;
    di::Optional<LocalApicNmiStructure> local_apic_nmi;
};

void init_acpi();
}
