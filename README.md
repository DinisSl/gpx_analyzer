# GPX Analyzer Documentation

This document describes the structure and functionality of [`gpx_analyzer.c`](running/gpx_analyzer.c), a C program for analyzing GPX (GPS Exchange Format) files, calculating running splits, and generating reports and plots.

---

## Overview

The program parses a GPX file, extracts track points (latitude, longitude, and timestamp), calculates distance and time splits, computes pace metrics, generates a summary report, and exports the track points to a JSON file. It also plots split times using Gnuplot.

---

## Main Components

### 1. **Memory and File Utilities**
- `safe_malloc(size_t size)`: Allocates memory, exits on failure.
- `safe_fopen(const char* filename, const char* mode)`: Opens a file, exits on failure.

### 2. **XML Parsing**
- Uses Expat XML parser.
- **Handlers:**
  - `start_element`: Handles `<trkpt>` and `<time>` elements, extracts latitude, longitude, and prepares for time extraction.
  - `end_element`: Finalizes track points and parses time.
  - `char_data`: Collects character data for time elements.

### 3. **GPX Parsing**
- `parse_gpx_file(const char* file_path, size_t* out_num_points)`: 
  - Opens and parses the GPX file.
  - Returns an array of `TrackPoint` structs.

### 4. **Distance Calculation**
- `geodesic_distance(double lat1, double lon1, double lat2, double lon2)`: 
  - Uses the geodesic library for accurate distance calculation between two coordinates.

### 5. **Time Parsing**
- `parse_time(const char *time_str)`: 
  - Parses ISO 8601 time strings from GPX files into seconds since epoch.

### 6. **Split Calculation**
- `calculate_splits(const TrackPoint* points, size_t num_points, int split_distance, size_t* split_count)`: 
  - Divides the track into segments of a given distance.
  - Handles edge cases (e.g., invalid time differences, excessive speed).

### 7. **Pace and Time Metrics**
- `convert_seconds(double total_seconds)`: Converts seconds to a `Time` struct (minutes, seconds).
- `calculate_pace(double total_time, double total_distance)`: Calculates pace per kilometer.
- `precompute_time_metrics(...)`: Precomputes split paces, cumulative times, and cumulative paces.

### 8. **Reporting and Plotting**
- `print_split_report(...)`: Prints a table of splits, times, and paces.
- `plot_splits(...)`: Plots split times using Gnuplot.
- `print_summary(...)`: Prints total distance, time, and average pace.

### 9. **JSON Export**
- `write_trackpoints_json(const char* filename, const TrackPoint* points, size_t count)`: 
  - Exports track points to a JSON file.

### 10. **Resource Cleanup**
- `cleanup_resources(...)`: Frees all allocated memory.

---

## Program Flow

1. **Argument Validation:** Checks for correct usage and valid split distance.
2. **File Path Construction:** Builds the GPX file path from arguments.
3. **GPX Parsing:** Loads track points from the GPX file.
4. **Split Calculation:** Calculates distance/time splits.
5. **Metric Precomputation:** Computes pace and cumulative metrics.
6. **Report Generation:** Prints split tables and summary, plots splits.
7. **JSON Export:** Writes track points to `track.json`.
8. **Cleanup:** Frees all allocated resources.

---

## Example Usage

```sh
./gpx_analyzer <gpx_file_name_without_extension> <split_distance_in_meters>
```

Example:
```sh
./gpx_analyzer tres_mil_1024 1000
```

---

## File Structure

- [`gpx_analyzer.c`](running/gpx_analyzer.c): Main implementation file.
- [`gpx_analyzer.h`](running/gpx_analyzer.h): Header file with type and function declarations.
- `gpx/`: Directory containing GPX files.
- `track.json`: Output JSON file with track points.

---

## Dependencies

- [Expat XML Parser](https://libexpat.github.io/)
- [GeographicLib](https://geographiclib.sourceforge.io/) (for geodesic calculations)
- [Gnuplot](http://www.gnuplot.info/) (for plotting splits)

---

## See Also

- [gpx_analyzer.h](running/gpx_analyzer.h) for type definitions and function declarations.
- [README.md](running/README.md) for an execution tree and further details.
