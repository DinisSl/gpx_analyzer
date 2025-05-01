#ifndef GPX_ANALYZER_H
#define GPX_ANALYZER_H

#include "geodesic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <expat.h>

// Constants
#define MAX_SPEED 15.0
#define MIN_TIME_DIFFERENCE 0.1
#define BUFFER_SIZE 256
#define FILE_PATH_SIZE 256
#define MAX_TAG_LEN 128
#define MAX_POINTS 10000
#define MAX_SPLITS 1000

// Macros
#define CHECK_ERROR(condition, cleanup_label) \
    if (!(condition)) { goto cleanup_label; }

// Data Structures
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
    TrackPoint *points;
    size_t num_points, capacity;
    bool in_trkpt, in_time;
    char tag[MAX_TAG_LEN];
    char buffer[BUFFER_SIZE];
    TrackPoint current_point;
} ParserState;

// Core Processing Functions
TrackPoint* parse_gpx_file(const char* file_path, size_t* out_num_points);
bool load_track_points(const char* file_path, TrackPoint** points, size_t* num_points);
bool calculate_and_validate_splits(TrackPoint* points, size_t num_points, int split_distance,
                                   Split** splits, size_t* split_count);
bool precompute_time_metrics(Split* splits, size_t split_count, PrecomputedTimes* precomputed);
void generate_reports(TrackPoint* points, size_t num_points, Split* splits, size_t split_count,
                      PrecomputedTimes* precomputed, int split_distance);
void cleanup_resources(TrackPoint* track_points, Split* splits, PrecomputedTimes* precomputed);

// Geographic Calculations
double geodesic_distance(double lat1, double lon1, double lat2, double lon2);

// Time Conversion Functions
double parse_time(const char *time_str);
Time convert_seconds(double total_seconds);
void format_time(char* buffer, size_t size, Time t);

// Split and Metrics Functions
Split* calculate_splits(const TrackPoint* points, size_t num_points, int split_distance, size_t* split_count);
double calculate_total_distance(const TrackPoint *points, size_t num_points);
Time calculate_pace(double total_time, double total_distance);

// Report and Visualization Functions
void print_split_report(const Split *splits, const Time *split_paces, const Time *cumulative_times,
                        const Time *cumulative_paces, size_t split_count);
void plot_splits(const Split* splits, size_t split_count, int split_distance);
void print_summary(double total_distance, Time total_time_t, Time pace);

// JSON Export Function
void write_trackpoints_json(const char* filename, const TrackPoint* points, size_t count);

// Helper and Utility Functions
bool process_arguments(int argc, char* argv[], char* file_path, size_t path_size);

#endif // GPX_ANALYZER_H