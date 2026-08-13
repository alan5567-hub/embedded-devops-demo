#include <stdio.h>
#include "vehicle.h"

/**
 * T-Box Diagnostic Simulation
 *
 * Entry point: initialises a VehicleTelemetry instance, simulates a sensor
 * read, and prints the resulting diagnostic report to the console.
 */
int main(void)
{
    VehicleTelemetry telemetry;

    printf("\n[T-Box] Initialising vehicle telemetry system...\n\n");

    /* Step 1: Initialise with safe defaults */
    vehicle_init(&telemetry);

    /* Step 2: Simulate reading from vehicle sensors */
    vehicle_simulate_reading(&telemetry);

    /* Step 3: Print the diagnostic report */
    vehicle_print_telemetry(&telemetry);

    printf("\n[T-Box] Diagnostic cycle complete.\n\n");
    vehicle_print_telemetry(&telemetry);

    return 0;
}
