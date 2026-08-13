#ifndef VEHICLE_H
#define VEHICLE_H

/* Vehicle telemetry data structure */
typedef struct {
    float speed_kmh;       /* Current vehicle speed in km/h */
    float battery_level;   /* Battery level as a percentage (0-100) */
    float temperature_c;   /* Ambient/engine temperature in Celsius */
    int   engine_on;       /* Engine status: 1 = ON, 0 = OFF */
} VehicleTelemetry;

/* Vehicle status codes */
typedef enum {
    STATUS_NORMAL   = 0,
    STATUS_WARNING  = 1,
    STATUS_CRITICAL = 2
} VehicleStatus;

/* Function declarations */
void          vehicle_init(VehicleTelemetry *telemetry);
void          vehicle_simulate_reading(VehicleTelemetry *telemetry);
VehicleStatus vehicle_get_status(const VehicleTelemetry *telemetry);
const char   *vehicle_status_to_string(VehicleStatus status);
void          vehicle_print_telemetry(const VehicleTelemetry *telemetry);

#endif /* VEHICLE_H */
