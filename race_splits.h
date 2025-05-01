#ifndef RACE_SPLITS_H
#define RACE_SPLITS_H

#include <stddef.h>  // for size_t

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

// Constants
#define MAX_SPEED 15.0         // Maximum allowed speed (m/s)
#define MIN_TIME_DIFF 0.1        // Minimum time difference (s)

// Data structures
typedef struct {
    double lat;
    double lon;
    double time;  // Seconds since epoch
} TrackPoint;

typedef struct {
    int minutes;
    int seconds;
} Time;

// Function prototypes
double haversine(double lat1, double lon1, double lat2, double lon2);
double parse_time(const char *time_str);
Time convert_seconds(int total_seconds);
TrackPoint* read_track_points(const char *file_path, size_t *num_points);
void compute_and_print_splits(TrackPoint *points, size_t num_points, int split_distance, double *total_distance, double *total_time);
void plot_splits(Time *split_times, double *split_distances, size_t split_count, int split_distance);
void setup_utc_timezone(void);

#endif /* RACE_SPLITS_H */
