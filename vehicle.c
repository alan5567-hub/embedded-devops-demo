#include "vehicle.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Simulated base values for telemetry readings */
#define BASE_SPEED       80.0f
#define BASE_BATTERY     75.0f
#define BASE_TEMP        35.0f
#define SPEED_VARIANCE   40.0f
#define BATTERY_VARIANCE 20.0f
#define TEMP_VARIANCE    15.0f

/* Thresholds for status evaluation */
#define BATTERY_WARN_THRESHOLD     20.0f
#define BATTERY_CRITICAL_THRESHOLD 10.0f
#define TEMP_WARN_THRESHOLD        70.0f
#define TEMP_CRITICAL_THRESHOLD    90.0f

/**
 * @brief Initialise the telemetry struct with safe default values.
 */
void vehicle_init(VehicleTelemetry *telemetry)
{
    if (!telemetry) return;

    telemetry->speed_kmh     = 0.0f;
    telemetry->battery_level = 100.0f;
    telemetry->temperature_c = 25.0f;
    telemetry->engine_on     = 0;
}

/**
 * @brief Populate the telemetry struct with simulated sensor readings.
 *
 * Uses a seeded random number to mimic fluctuating sensor data without
 * requiring actual hardware.
 */
void vehicle_simulate_reading(VehicleTelemetry *telemetry)
{
    if (!telemetry) return;

    /* Seed only once per session */
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    /* Generate values within realistic ranges */
    float rand_ratio = (float)rand() / (float)RAND_MAX;  /* 0.0 – 1.0 */

    telemetry->speed_kmh     = BASE_SPEED     + (rand_ratio * SPEED_VARIANCE)   - (SPEED_VARIANCE   / 2.0f);
    telemetry->battery_level = BASE_BATTERY   + (rand_ratio * BATTERY_VARIANCE) - (BATTERY_VARIANCE / 2.0f);
    telemetry->temperature_c = BASE_TEMP      + (rand_ratio * TEMP_VARIANCE)    - (TEMP_VARIANCE    / 2.0f);
    telemetry->engine_on     = (telemetry->speed_kmh > 0.0f) ? 1 : 0;

    /* Clamp values to valid ranges */
    if (telemetry->speed_kmh     < 0.0f)   telemetry->speed_kmh     = 0.0f;
    if (telemetry->battery_level < 0.0f)   telemetry->battery_level = 0.0f;
    if (telemetry->battery_level > 100.0f) telemetry->battery_level = 100.0f;
}

/**
 * @brief Derive an overall vehicle status based on current telemetry.
 */
VehicleStatus vehicle_get_status(const VehicleTelemetry *telemetry)
{
    if (!telemetry) return STATUS_CRITICAL;

    if (telemetry->battery_level <= BATTERY_CRITICAL_THRESHOLD ||
        telemetry->temperature_c >= TEMP_CRITICAL_THRESHOLD) {
        return STATUS_CRITICAL;
    }

    if (telemetry->battery_level <= BATTERY_WARN_THRESHOLD ||
        telemetry->temperature_c >= TEMP_WARN_THRESHOLD) {
        return STATUS_WARNING;
    }

    return STATUS_NORMAL;
}

/**
 * @brief Convert a VehicleStatus enum value to a human-readable string.
 */
const char *vehicle_status_to_string(VehicleStatus status)
{
    switch (status) {
        case STATUS_NORMAL:   return "NORMAL";
        case STATUS_WARNING:  return "WARNING";
        case STATUS_CRITICAL: return "CRITICAL";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief Pretty-print all telemetry fields to stdout.
 */
void vehicle_print_telemetry(const VehicleTelemetry *telemetry)
{
    if (!telemetry) return;

    VehicleStatus status = vehicle_get_status(telemetry);

    printf("========================================\n");
    printf("  T-Box Diagnostic Report\n");
    printf("========================================\n");
    printf("  Engine       : %s\n",    telemetry->engine_on ? "ON" : "OFF");
    printf("  Speed        : %.1f km/h\n", telemetry->speed_kmh);
    printf("  Battery      : %.1f %%\n",   telemetry->battery_level);
    printf("  Temperature  : %.1f deg C\n", telemetry->temperature_c);
    printf("  Status       : %s\n",    vehicle_status_to_string(status));
    printf("========================================\n");
}
