# Execution Tree for `gpx_analyzer.c`

## **1. `main`**
**Purpose**: Entry point of the program.

### **Steps**:
1. **Initialize Variables**:
   - `track_points`, `splits`, `precomputed`, `file_path`, `num_points`, `split_distance`, `split_count`.

2. **Validate Arguments**:
   - Ensures the correct number of arguments is provided.
   - Validates that the split distance is a positive integer.

3. **Process Arguments**:
   - Calls `process_arguments` to construct the file path for the GPX file.

4. **Load Track Points**:
   - Calls `load_track_points`.
     - Internally calls `parse_gpx_file`.
       - Uses `start_element`, `end_element`, and `char_data` for XML parsing.

5. **Calculate Splits**:
   - Calls `calculate_and_validate_splits`.
     - Internally calls `calculate_splits`.
       - Uses `geodesic_distance` for distance calculations.

6. **Precompute Metrics**:
   - Calls `precompute_time_metrics`.
     - Uses `calculate_pace` and `convert_seconds` for time and pace calculations.

7. **Generate Reports**:
   - Calls `generate_reports`.
     - Internally calls:
       - `calculate_total_distance`.
       - `convert_seconds`.
       - `calculate_pace`.
       - `print_split_report`.
       - `plot_splits`.
       - `print_summary`.

8. **Write JSON**:
   - Calls `write_trackpoints_json` to export track points to a JSON file.

9. **Cleanup Resources**:
   - Calls `cleanup_resources` to free all allocated memory.

---

## **2. `process_arguments`**
**Purpose**: Validates and processes command-line arguments.

### **Steps**:
- Checks if `argc` is valid.
- Constructs the file path for the GPX file.

---

## **3. `load_track_points`**
**Purpose**: Loads track points from the GPX file.

### **Steps**:
1. Calls `parse_gpx_file`.
   - Opens the GPX file.
   - Initializes the XML parser (`expat`).
   - Reads the file in chunks and processes XML elements using:
     - **`start_element`**:
       - Handles `<trkpt>` and `<time>` elements.
       - Extracts latitude, longitude, and time.
     - **`end_element`**:
       - Finalizes track points when a `<trkpt>` element ends.
       - Resizes the `points` array dynamically.
     - **`char_data`**:
       - Captures text data (e.g., time values).
2. Returns the parsed track points.

---

## **4. `calculate_and_validate_splits`**
**Purpose**: Calculates and validates splits based on track points and split distance.

### **Steps**:
1. Calls `calculate_splits`.
   - Iterates through the track points.
   - Calculates distances between consecutive points using `geodesic_distance`.
   - Splits the track into segments based on the split distance.
   - Handles edge cases (e.g., invalid time differences or excessive speed).
2. Returns an array of splits.

---

## **5. `precompute_time_metrics`**
**Purpose**: Precomputes time metrics for splits.

### **Steps**:
1. Allocates memory for:
   - `split_paces`.
   - `cumulative_times`.
   - `cumulative_paces`.
2. Iterates through the splits to calculate:
   - Split paces using `calculate_pace`.
   - Cumulative times using `convert_seconds`.
   - Cumulative paces using `calculate_pace`.

---

## **6. `generate_reports`**
**Purpose**: Generates and displays reports for the track.

### **Steps**:
1. Calculates total distance using `calculate_total_distance`.
2. Calculates total time and average pace using `convert_seconds` and `calculate_pace`.
3. Calls:
   - **`print_split_report`**:
     - Prints a detailed table of splits, times, and paces.
   - **`plot_splits`**:
     - Uses Gnuplot to plot the splits.
   - **`print_summary`**:
     - Prints the total distance, time, and average pace.

---

## **7. `write_trackpoints_json`**
**Purpose**: Exports track points to a JSON file.

### **Steps**:
1. Opens the JSON file for writing.
2. Iterates through the track points.
   - Formats each point as a JSON object with `lat`, `lon`, and `time`.
3. Writes the JSON array to the file.

---

## **8. `cleanup_resources`**
**Purpose**: Frees all allocated memory.

### **Steps**:
- Frees:
  - `track_points`.
  - `splits`.
  - `precomputed->split_paces`.
  - `precomputed->cumulative_times`.
  - `precomputed->cumulative_paces`.

---

## **Detailed Function Dependencies**

### **`parse_gpx_file`**
- **Depends on**:
  - `start_element`.
  - `end_element`.
  - `char_data`.

### **`calculate_splits`**
- **Depends on**:
  - `geodesic_distance`.

### **`precompute_time_metrics`**
- **Depends on**:
  - `calculate_pace`.
  - `convert_seconds`.

### **`generate_reports`**
- **Depends on**:
  - `calculate_total_distance`.
  - `convert_seconds`.
  - `calculate_pace`.
  - `print_split_report`.
  - `plot_splits`.
  - `print_summary`.

### **`write_trackpoints_json`**
- **Depends on**:
  - JSON formatting logic.

---

## Expanded Execution Tree
main
+-- process_arguments
+-- load_track_points
|   +-- parse_gpx_file
|       +-- start_element
|       +-- end_element
|       +-- char_data
+-- calculate_and_validate_splits
|   +-- calculate_splits
|       +-- geodesic_distance
+-- precompute_time_metrics
|   +-- calculate_pace
|   +-- convert_seconds
+-- generate_reports
|   +-- calculate_total_distance
|   +-- convert_seconds
|   +-- calculate_pace
|   +-- print_split_report
|   +-- plot_splits
|   +-- print_summary
+-- write_trackpoints_json
+-- cleanup_resources