#ifndef GPX_ANALYZER_H
#define GPX_ANALYZER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <expat.h>
#include <geodesic.h>

#define MAX_POINTS 50000
#define MAX_TAG_LEN 32
#define BUFFER_SIZE 4096
#define FILE_PATH_SIZE 256
#define MIN_TIME_DIFFERENCE 0.1
#define MAX_SPEED 12.5  // meters per second (~45 km/h)

typedef struct {
    double lat;
    double lon;
    double time;
} TrackPoint;

typedef struct {
    int minutes;
    double seconds;
} Time;

typedef struct {
    Time time;
    double distance;
} Split;

typedef struct {
    Time* split_paces;
    Time* cumulative_times;
    Time* cumulative_paces;
    size_t count;
} PrecomputedTimes;

typedef struct {
    TrackPoint* points;
    size_t num_points;
    size_t capacity;
    TrackPoint current_point;
    bool in_trkpt;
    bool in_time;
    char tag[MAX_TAG_LEN];
    char buffer[BUFFER_SIZE];
} ParserState;

#define RETURN_IF_ERROR(condition, cleanup_code) \
    if (!(condition)) { \
        cleanup_code; \
        return false; \
    }

#define CHECK_ERROR(expr, label) \
    if (!(expr)) goto label

// Function declarations
TrackPoint* parse_gpx_file(const char* file_path, size_t* out_num_points);
Split* calculate_splits(const TrackPoint* points, size_t num_points, int split_distance, size_t* split_count);
void generate_full_report(const Split* splits, size_t split_count, const PrecomputedTimes* precomputed,
                         double total_distance, double total_time_seconds, int split_distance);
bool process_arguments(int argc, char* argv[], char* file_path, size_t path_size);
bool load_track_points(const char* file_path, TrackPoint** points, size_t* num_points);
bool calculate_and_validate_splits(TrackPoint* points, size_t num_points, int split_distance,
                                 Split** splits, size_t* split_count);
bool precompute_time_metrics(Split* splits, size_t split_count, PrecomputedTimes* precomputed);
void cleanup_resources(TrackPoint* track_points, Split* splits, PrecomputedTimes* precomputed);
double parse_time(const char* time_str);
Time calculate_pace(double total_time, double total_distance);
Time convert_seconds(double total_seconds);
void format_time(char* buffer, size_t size, Time t);

#endif