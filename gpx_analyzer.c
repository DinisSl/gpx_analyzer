#include "gpx_analyzer.h"

static void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static FILE* safe_fopen(const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s.\n", filename);
        exit(EXIT_FAILURE);
    }
    return fp;
}

static void handle_xml_error(XML_Parser parser, FILE* file, void* points) {
    fprintf(stderr, "Error: XML parse error at line %lu: %s\n",
            XML_GetCurrentLineNumber(parser),
            XML_ErrorString(XML_GetErrorCode(parser)));
    XML_ParserFree(parser);
    fclose(file);
    free(points);
    exit(EXIT_FAILURE);
}

static void start_element(void *data, const char *el, const char **attr) {
    ParserState *state = (ParserState*)data;
    strncpy(state->tag, el, MAX_TAG_LEN - 1);
    state->tag[MAX_TAG_LEN - 1] = '\0';

    if (strcmp(el, "trkpt") == 0 || strcmp(el, "gpx:trkpt") == 0) {
        state->in_trkpt = true;
        for (int i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "lat") == 0) {
                state->current_point.lat = strtod(attr[i + 1], NULL);
            } else if (strcmp(attr[i], "lon") == 0) {
                state->current_point.lon = strtod(attr[i + 1], NULL);
            }
        }
    } else if (state->in_trkpt &&
               (strcmp(el, "time") == 0 || strcmp(el, "gpx:time") == 0)) {
        state->in_time = true;
        state->buffer[0] = '\0';
    }
}

static void end_element(void *data, const char *el) {
    ParserState *state = (ParserState*)data;
    if (state->in_time &&
        (strcmp(el, "time") == 0 || strcmp(el, "gpx:time") == 0)) {
        state->current_point.time = parse_time(state->buffer);
        state->in_time = false;
    } else if (state->in_trkpt &&
               (strcmp(el, "trkpt") == 0 || strcmp(el, "gpx:trkpt") == 0)) {
        if (state->num_points >= state->capacity) {
            state->capacity *= 2;
            TrackPoint* tmp = realloc(state->points, state->capacity * sizeof(TrackPoint));
            if (!tmp) {
                fprintf(stderr, "Error: Memory reallocation failed.\n");
                exit(EXIT_FAILURE);
            }
            state->points = tmp;
        }
        state->points[state->num_points++] = state->current_point;
        state->in_trkpt = false;
    }
}

static void char_data(void *data, const char *content, int len) {
    ParserState *state = (ParserState*)data;
    if (state->in_time) {
        strncat(state->buffer, content, len);
    }
}

TrackPoint* parse_gpx_file(const char* file_path, size_t* out_num_points) {
    FILE *file = safe_fopen(file_path, "r");
    XML_Parser parser = XML_ParserCreate(NULL);
    
    if (!parser) {
        fprintf(stderr, "Error: Could not create XML parser.\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    ParserState state = {
        .points = safe_malloc(MAX_POINTS * sizeof(TrackPoint)),
        .capacity = MAX_POINTS
    };

    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, char_data);

    char buffer[BUFFER_SIZE];
    size_t len;
    int done;

    do {
        len = fread(buffer, 1, sizeof(buffer), file);
        done = len < sizeof(buffer);
        if (XML_Parse(parser, buffer, (int)len, done) == XML_STATUS_ERROR) {
            handle_xml_error(parser, file, state.points);
        }
    } while (!done);

    XML_ParserFree(parser);
    fclose(file);

    *out_num_points = state.num_points;
    return state.points;
}

double geodesic_distance(double lat1, double lon1, double lat2, double lon2) {
    static struct geod_geodesic g;
    static int initialized = 0;
    double s12, azi1, azi2;

    if (!initialized) {
        geod_init(&g, 6378137, 1/298.257223563);
        initialized = 1;
    }

    geod_inverse(&g, lat1, lon1, lat2, lon2, &s12, &azi1, &azi2);
    return s12;
}

// Time parsing left unchanged
double parse_time(const char *time_str) {
    struct tm tm = {0};
    int milliseconds = 0;
    char *end;
    const char *ptr = time_str;

    tm.tm_year = strtol(ptr, &end, 10) - 1900;
    if (*end != '-') return 0.0;
    ptr = end + 1;

    tm.tm_mon = strtol(ptr, &end, 10) - 1;
    if (*end != '-') return 0.0;
    ptr = end + 1;

    tm.tm_mday = strtol(ptr, &end, 10);
    if (*end != 'T') return 0.0;
    ptr = end + 1;

    tm.tm_hour = strtol(ptr, &end, 10);
    if (*end != ':') return 0.0;
    ptr = end + 1;

    tm.tm_min = strtol(ptr, &end, 10);
    if (*end != ':') return 0.0;
    ptr = end + 1;

    tm.tm_sec = strtol(ptr, &end, 10);
    if (*end == '.') {
        ptr = end + 1;
        milliseconds = strtol(ptr, &end, 10);
    }
    if (*end != 'Z') return 0.0;

#ifndef _WIN32
    time_t t = timegm(&tm);
#else
    _putenv("TZ=UTC");
    _tzset();
    time_t t = mktime(&tm);
#endif
    return (double)t + milliseconds / 1000.0;
}

Time convert_seconds(double total_seconds) {
    int minutes = (int)(total_seconds / 60);
    double seconds = total_seconds - (double)minutes * 60;
    return (Time){ minutes, seconds };
}

void format_time(char* buffer, size_t size, Time t) {
    snprintf(buffer, size, "%02d:%05.2f", t.minutes, t.seconds);
}

// Combined argument check and path build
bool process_arguments(int argc, char* argv[], char* file_path, size_t path_size) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <gpx_file> <split_distance>\n", argv[0]);
        return false;
    }
    snprintf(file_path, path_size,
             "C:\\Users\\dinis\\Coding\\VSCode\\gpx\\%s",
             argv[1]);
    return true;
}

bool load_track_points(const char* file_path, TrackPoint** points, size_t* num_points) {
    *points = parse_gpx_file(file_path, num_points);
    if (!*points || *num_points < 2) {
        fprintf(stderr, "Error: Invalid or insufficient GPX data\n");
        return false;
    }
    return true;
}

bool calculate_and_validate_splits(TrackPoint* points, size_t num_points, int split_distance,
                                   Split** splits, size_t* split_count) {
    *splits = calculate_splits(points, num_points, split_distance, split_count);
    if (!*splits || *split_count == 0) {
        fprintf(stderr, "Error calculating splits\n");
        return false;
    }
    return true;
}

bool precompute_time_metrics(Split* splits, size_t split_count, PrecomputedTimes* precomputed) {
    precomputed->split_paces = malloc(split_count * sizeof(Time));
    precomputed->cumulative_times = malloc(split_count * sizeof(Time));
    precomputed->cumulative_paces = malloc(split_count * sizeof(Time));

    if (!precomputed->split_paces || !precomputed->cumulative_times || !precomputed->cumulative_paces) {
        free(precomputed->split_paces);
        free(precomputed->cumulative_times);
        free(precomputed->cumulative_paces);
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }

    double cumulative_time_seconds = 0.0;
    for (size_t i = 0; i < split_count; i++) {
        double split_seconds = splits[i].time.minutes * 60.0 + splits[i].time.seconds;
        cumulative_time_seconds += split_seconds;

        double segment_dist = (i == 0) ? splits[i].distance : splits[i].distance - splits[i-1].distance;

        precomputed->split_paces[i] = calculate_pace(split_seconds, segment_dist);
        precomputed->cumulative_times[i] = convert_seconds(cumulative_time_seconds);
        precomputed->cumulative_paces[i] = calculate_pace(cumulative_time_seconds, splits[i].distance);
    }

    precomputed->count = split_count;
    return true;
}

// Split calculation remains as before, using geodesic_distance
Split* calculate_splits(const TrackPoint* points, size_t num_points, int split_distance, size_t* split_count) {
    double cumulative = 0.0;
    double split_start_time = points[0].time;
    double total_end_time = points[num_points - 1].time;
    Split* splits = NULL;
    *split_count = 0;

    for (size_t i = 1; i < num_points; i++) {
        double d = geodesic_distance(points[i-1].lat, points[i-1].lon,
                                     points[i].lat, points[i].lon);
        double t = points[i].time - points[i-1].time;

        if (t <= MIN_TIME_DIFFERENCE || (t > 0 && d/t > MAX_SPEED))
            continue;

        double remaining = d;
        while (remaining > 0) {
            double needed = (double)split_distance - cumulative;
            if (remaining >= needed) {
                double fraction = needed / remaining;
                double split_time = (points[i-1].time + fraction * t) - split_start_time;

                Split* tmp = realloc(splits, (*split_count + 1) * sizeof(Split));
                if (!tmp) return NULL;
                splits = tmp;
                splits[*split_count].time = convert_seconds(split_time);
                splits[*split_count].distance = (double)(*split_count + 1) * split_distance;
                (*split_count)++;

                split_start_time += split_time;
                cumulative = 0.0;
                remaining -= needed;
            } else {
                cumulative += remaining;
                remaining = 0;
            }
        }
    }

    if (cumulative > 0.0) {
        double leftover_time = total_end_time - split_start_time;
        Split* tmp = realloc(splits, (*split_count + 1) * sizeof(Split));
        if (!tmp) return NULL;
        splits = tmp;
        splits[*split_count].time = convert_seconds(leftover_time);
        splits[*split_count].distance = (double)(*split_count * split_distance) + round(cumulative);
        (*split_count)++;
    }

    return splits;
}

// Report generation & printing
void print_split_report(
        const Split *splits,
        const Time *split_paces,
        const Time *cumulative_times,
        const Time *cumulative_paces,
        size_t split_count
) {
    printf("   Split    cumulative |    split   |     split     | cumulative |   cumulative  | cumulative \n");
    printf("             distance  |    time    |     pace      |    time    |      pace     |  distance  \n");
    printf("==============================================================================================\n");

    for (size_t i = 0; i < split_count; i++) {
        printf("Split %2zu-> %7.0fm    | %2d'%05.2f'' | %2d'%05.2f''/Km | %2d'%05.2f'' | %2d'%05.2f''/Km | %6.0fm\n",
               i + 1,
               splits[i].distance,
               splits[i].time.minutes, splits[i].time.seconds,
               split_paces[i].minutes, split_paces[i].seconds,
               cumulative_times[i].minutes, cumulative_times[i].seconds,
               cumulative_paces[i].minutes, cumulative_paces[i].seconds,
               splits[i].distance);
    }
}

void plot_splits(const Split* splits, size_t split_count, int split_distance) {
    FILE *gnuplot = popen("gnuplot -persistent", "w");
    if (!gnuplot) return;

    fprintf(gnuplot, "set title 'Split Time (%dm)'\n", split_distance);
    fprintf(gnuplot, "set xlabel 'Distance (m)'\n");

    if (split_distance >= 500) {
        fprintf(gnuplot, "set ylabel 'Time (mm:ss)'\n");
        fprintf(gnuplot, "set timefmt '%%M:%%S'\n");
        fprintf(gnuplot, "set ydata time\n");
        // <<< add this to invert the Y axis
        fprintf(gnuplot, "set yrange [*:*] reverse\n");
        fprintf(gnuplot, "plot '-' using 1:(strptime('%%M:%%S', strcol(2))) "
                         "with linespoints title 'Splits'\n");
    } else {
        fprintf(gnuplot, "set ylabel 'Time (s)'\n");
        // <<< same inversion here
        fprintf(gnuplot, "set yrange [*:*] reverse\n");
        fprintf(gnuplot, "plot '-' using 1:2 with linespoints title 'Splits'\n");
    }

    for (size_t i = 0; i < split_count - 1; i++) {
        if (split_distance >= 500) {
            char time_str[16];
            format_time(time_str, sizeof(time_str), splits[i].time);
            fprintf(gnuplot, "%.0f \"%s\"\n", splits[i].distance, time_str);
        } else {
            fprintf(gnuplot, "%.0f %.2f\n", splits[i].distance,
                    splits[i].time.minutes * 60 + splits[i].time.seconds);
        }
    }
    fprintf(gnuplot, "e\n");
    pclose(gnuplot);
}


double calculate_total_distance(const TrackPoint *points, size_t num_points) {
    double total_distance = 0.0;
    for (size_t i = 1; i < num_points; i++) {
        double d = geodesic_distance(points[i-1].lat, points[i-1].lon,
                                     points[i].lat, points[i].lon);
        double t = points[i].time - points[i-1].time;

        if (t > MIN_TIME_DIFFERENCE && (d/t) <= MAX_SPEED) {
            total_distance += d;
        }
    }
    return total_distance;
}

Time calculate_pace(double total_time, double total_distance) {
    if (total_distance == 0) return (Time){0, 0.0};
    double pace_sec_per_km = total_time / (total_distance / 1000.0);
    return convert_seconds(pace_sec_per_km);
}

void print_summary(double total_distance, Time total_time_t, Time pace) {
    printf("\nTotal distance: %.0fm\n", total_distance);
    printf("Total Time: %d'%05.2f''\n", total_time_t.minutes, total_time_t.seconds);
    printf("Average Pace: %d'%05.2f''/Km\n", pace.minutes, pace.seconds);
}

void generate_reports(TrackPoint* points, size_t num_points, Split* splits, size_t split_count,
                      PrecomputedTimes* precomputed, int split_distance) {
    double total_distance = calculate_total_distance(points, num_points);
    double total_time_seconds = points[num_points-1].time - points[0].time;

    print_split_report(
            splits,
            precomputed->split_paces,
            precomputed->cumulative_times,
            precomputed->cumulative_paces,
            split_count
    );

    plot_splits(splits, split_count, split_distance);

    Time total_time_t = convert_seconds(total_time_seconds);
    Time pace = calculate_pace(total_time_seconds, total_distance);
    print_summary(total_distance, total_time_t, pace);
}

void write_trackpoints_json(const char* filename, const TrackPoint* points, size_t count) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Error opening JSON file");
        return;
    }

    fprintf(fp, "[\n");

    for (size_t i = 0; i < count; ++i) {
        char iso_time[32];
        time_t raw_time = (time_t)points[i].time;
        struct tm *utc_time = gmtime(&raw_time);
        strftime(iso_time, sizeof(iso_time), "%Y-%m-%dT%H:%M:%SZ", utc_time);

        fprintf(fp,
                "  {\"lat\": %.8f, \"lon\": %.8f, \"time\": \"%s\"}%s\n",
                points[i].lat,
                points[i].lon,
                iso_time,
                (i < count - 1) ? "," : ""
        );
    }

    fprintf(fp, "]\n");
    fclose(fp);
}

void cleanup_resources(TrackPoint* track_points, Split* splits, PrecomputedTimes* precomputed) {
    free(track_points);
    free(splits);
    free(precomputed->split_paces);
    free(precomputed->cumulative_times);
    free(precomputed->cumulative_paces);
}

int main(int argc, char* argv[]) {
    TrackPoint* track_points = NULL;
    Split* splits = NULL;
    PrecomputedTimes precomputed = { NULL, NULL, NULL, 0 };

    char file_path[FILE_PATH_SIZE];
    size_t num_points = 0;
    size_t split_count = 0;
    int split_distance = 0;

    // Validate arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <gpx_file> <split_distance>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse split distance
    char *endptr;
    split_distance = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || split_distance <= 0) {
        fprintf(stderr, "Error: Invalid split distance provided.\n");
        return EXIT_FAILURE;
    }

    // Process arguments and load data
    CHECK_ERROR(process_arguments(argc, argv, file_path, sizeof(file_path)), cleanup);
    CHECK_ERROR(load_track_points(file_path, &track_points, &num_points), cleanup);
    CHECK_ERROR(calculate_and_validate_splits(track_points, num_points, split_distance, &splits, &split_count), cleanup);
    CHECK_ERROR(precompute_time_metrics(splits, split_count, &precomputed), cleanup);

    // Generate reports
    generate_reports(track_points, num_points, splits, split_count, &precomputed, split_distance);

    // Write JSON file
    printf("Writing JSON to file...\n");
    write_trackpoints_json("track.json", track_points, num_points);

    // Cleanup and exit
    cleanup_resources(track_points, splits, &precomputed);
    return EXIT_SUCCESS;

cleanup:
    cleanup_resources(track_points, splits, &precomputed);
    return EXIT_FAILURE;
}