// Done with deepseek R1, gemini and chatGPT and refactored with chatGPT
#include "race_splits.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// ---------------------------------------------------------------------
// Helper function: Set up the environment for UTC timezone
void setup_utc_timezone(void) {
#ifdef _WIN32
    _putenv("TZ=UTC");
    _tzset();
#else
    setenv("TZ", "UTC", 1);
    tzset();
#endif
}

// ---------------------------------------------------------------------
// Calculates the great-circle distance between two coordinates 
// using the haversine formula.
double haversine(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371e3;
    double phi1 = lat1 * M_PI / 180.0;
    double phi2 = lat2 * M_PI / 180.0;
    double delta_phi = (lat2 - lat1) * M_PI / 180.0;
    double delta_lambda = (lon2 - lon1) * M_PI / 180.0;

    double a = sin(delta_phi / 2) * sin(delta_phi / 2) +
               cos(phi1) * cos(phi2) * sin(delta_lambda / 2) * sin(delta_lambda / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}

// ---------------------------------------------------------------------
// Parses an ISO time string to seconds since the epoch.
double parse_time(const char *time_str) {
    struct tm tm = {0};
    int milliseconds = 0;
    char buffer[64];
    strncpy(buffer, time_str, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    int items_parsed = sscanf(buffer, "%d-%d-%dT%d:%d:%d.%dZ",
                                &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                                &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &milliseconds);
    if (items_parsed < 6) {
        items_parsed = sscanf(buffer, "%d-%d-%dT%d:%d:%dZ",
                              &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                              &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
        if (items_parsed < 6) {
            fprintf(stderr, "Unrecognized time format: %s\n", time_str);
            return 0.0;
        }
    }
    tm.tm_year -= 1900;
    tm.tm_mon  -= 1;
    setup_utc_timezone();
    time_t t = mktime(&tm);
    return (double)t + (milliseconds / 1000.0);
}

// ---------------------------------------------------------------------
// Converts seconds into minutes and seconds.
Time convert_seconds(int total_seconds) {
    Time t;
    t.minutes = total_seconds / 60;
    t.seconds = total_seconds % 60;
    return t;
}

// ---------------------------------------------------------------------
// Reads track points from a GPX file.
TrackPoint* read_track_points(const char *file_path, size_t *num_points) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }
    
    TrackPoint *points = NULL;
    *num_points = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "<trkpt") || strstr(line, "<gpx:trkpt")) {
            double lat, lon;
            if (sscanf(line, "%*[^\"]\"%lf\"%*[^\"]\"%lf\"", &lat, &lon) != 2)
                continue;
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "<time>") || strstr(line, "<gpx:time>")) {
                    char *start = strchr(line, '>');
                    if (start) {
                        start++;
                        char *end = strchr(start, '<');
                        if (end) {
                            *end = '\0';
                            TrackPoint pt;
                            pt.lat = lat;
                            pt.lon = lon;
                            pt.time = parse_time(start);
                            
                            TrackPoint *temp = realloc(points, ((*num_points) + 1) * sizeof(TrackPoint));
                            if (!temp) {
                                fprintf(stderr, "Memory allocation error\n");
                                free(points);
                                fclose(file);
                                return NULL;
                            }
                            points = temp;
                            points[(*num_points)++] = pt;
                        }
                    }
                    break;
                }
            }
        }
    }
    fclose(file);
    return points;
}

// ---------------------------------------------------------------------
// Computes split times and plots the splits.
void compute_and_print_splits(TrackPoint *points, size_t num_points, int split_distance, double *total_distance, double *total_time) {
    double cumulative_distance = 0.0;
    double whole_time = 0.0;
    double split_start_time = points[0].time;
    *total_distance = 0.0;
    *total_time = points[num_points - 1].time - points[0].time;
    
    Time *split_times = NULL;
    double *split_distances = NULL;
    size_t split_count = 0;
    
    for (size_t i = 1; i < num_points; i++) {
        double distance = haversine(points[i - 1].lat, points[i - 1].lon, points[i].lat, points[i].lon);
        double time_delta = points[i].time - points[i - 1].time;
        if (time_delta <= MIN_TIME_DIFF || (time_delta > 0 && distance / time_delta > MAX_SPEED))
            continue;
        *total_distance += distance;
        double segment_distance = distance;
        double segment_time = time_delta;
        
        while (segment_distance > 0) {
            double remaining_distance = split_distance - cumulative_distance;
            if (segment_distance >= remaining_distance) {
                double fraction = remaining_distance / segment_distance;
                double split_end_time = points[i - 1].time + fraction * segment_time;
                double split_time = split_end_time - split_start_time;
                whole_time += split_time;
                double current_split_distance = (double)(split_count + 1) * split_distance;
                Time current_split = convert_seconds((int)split_time);
                
                Time *temp_times = realloc(split_times, (split_count + 1) * sizeof(Time));
                double *temp_distances = realloc(split_distances, (split_count + 1) * sizeof(double));
                if (!temp_times || !temp_distances) {
                    fprintf(stderr, "Memory allocation error\n");
                    free(split_times);
                    free(split_distances);
                    return;
                }
                split_times = temp_times;
                split_distances = temp_distances;
                split_times[split_count] = current_split;
                split_distances[split_count] = current_split_distance;
                split_count++;
                
                printf("Split %zu: Distance %.0fm, Split Time: %d'%02d'' , Whole Time: %.2fs\n",
                       split_count, current_split_distance, current_split.minutes, current_split.seconds, whole_time);
                
                split_start_time = split_end_time;
                cumulative_distance = 0.0;
                segment_distance -= remaining_distance;
                segment_time *= (1 - fraction);
            } else {
                cumulative_distance += segment_distance;
                segment_distance = 0;
            }
        }
    }
    
    plot_splits(split_times, split_distances, split_count, split_distance);
    
    free(split_times);
    free(split_distances);
}

// ---------------------------------------------------------------------
// Plots the split times using gnuplot.
void plot_splits(Time *split_times, double *split_distances, size_t split_count, int split_distance) {
    FILE *gnuplotPipe = popen("gnuplot -persistent", "w");
    if (!gnuplotPipe) {
        fprintf(stderr, "Failed to open gnuplot pipe.\n");
        return;
    }
    
    fprintf(gnuplotPipe, "set title 'Split Time (%dm) vs Distance'\n", split_distance);
    fprintf(gnuplotPipe, "set xlabel 'Distance (m)'\n");
    if (split_distance >= 500) {
        fprintf(gnuplotPipe, "set ydata time\n");
        fprintf(gnuplotPipe, "set timefmt '%%M:%%S'\n");
        fprintf(gnuplotPipe, "set format y '%%M:%%S'\n");
        fprintf(gnuplotPipe, "set ylabel 'Split Time (min:sec)'\n");
    } else {
        fprintf(gnuplotPipe, "set ylabel 'Split Time (s)'\n");
    }
    
    fprintf(gnuplotPipe, "plot '-' using 1:2 with linespoints title 'Splits'\n");
    for (size_t i = 0; i < split_count; i++) {
        if (split_distance >= 500) {
            char time_str[16];
            snprintf(time_str, sizeof(time_str), "%02d:%02d", split_times[i].minutes, split_times[i].seconds);
            fprintf(gnuplotPipe, "%lf %s\n", split_distances[i], time_str);
        } else {
            int split_time_seconds = split_times[i].minutes * 60 + split_times[i].seconds;
            fprintf(gnuplotPipe, "%lf %d\n", split_distances[i], split_time_seconds);
        }
    }
    fprintf(gnuplotPipe, "e\n");
    fflush(gnuplotPipe);
    pclose(gnuplotPipe);
}

// ---------------------------------------------------------------------
// Main function.
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <filename> <split_distance>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "C:\\Users\\dinis\\OneDrive\\Documentos\\VSCode\\UserDinis\\running\\gpx\\%s.gpx", argv[1]);
    
    size_t num_points = 0;
    TrackPoint *points = read_track_points(file_path, &num_points);
    if (!points)
        return EXIT_FAILURE;
    
    if (num_points < 2) {
        fprintf(stderr, "Insufficient data. Found %zu points.\n", num_points);
        free(points);
        return EXIT_FAILURE;
    }
    
    int split_distance = atoi(argv[2]);
    double total_distance = 0.0;
    double total_time = 0.0;
    
    compute_and_print_splits(points, num_points, split_distance, &total_distance, &total_time);
    
    printf("\nTotal Distance: %.2f meters\n", total_distance);
    Time total_time_struct = convert_seconds((int)total_time);
    printf("Total Time: %d'%02d''\n", total_time_struct.minutes, total_time_struct.seconds);
    
    double pace_sec_per_km = total_time / (total_distance / 1000.0);
    Time pace = convert_seconds((int)pace_sec_per_km);
    printf("Average Pace: %d'%02d''/Km\n", pace.minutes, pace.seconds);
    
    free(points);
    return EXIT_SUCCESS;
}
