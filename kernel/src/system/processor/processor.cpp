#include <system/processor/processor.hpp>

#include <logger/logger.hpp>

bool processorHasAPIC()
{
    unsigned int eax, unused, edx;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_APIC;
}

bool processorHasSSE()
{
    unsigned int eax, unused, edx;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_SSE;
}

bool processorEnableSSE()
{
    if (processorHasSSE())
    {
        _processorEnableSSE();

        // Print information
        unsigned int eax, unused, ecx, edx;
        __get_cpuid(1, &eax, &unused, &ecx, &edx);

        logInfon("%! SSE has been enabled.", "[Processor]");
        logInfon("\t- SSE2 %s supported.", (edx & CPUID_FEAT_EDX_SSE2) ? "is" : "not");
        logInfon("\t- SSE3 %s supported.", (ecx & CPUID_FEAT_ECX_SSE3) ? "is" : "not");
        logInfon("\t- SSSE3 %s supported.", (ecx & CPUID_FEAT_ECX_SSSE3) ? "is" : "not");
        logInfon("\t- SSE4.1 %s supported.", (ecx & CPUID_FEAT_ECX_SSE4_1) ? "is" : "not");
        logInfon("\t- SSE4.2 %s supported.", (ecx & CPUID_FEAT_ECX_SSE4_2) ? "is" : "not");

        return true;
    }
    return false;
}