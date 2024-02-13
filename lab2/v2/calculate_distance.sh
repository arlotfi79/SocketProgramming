#!/bin/bash

# Define function to calculate distance in kilometers
calculate_distance_km() {
    # Speed of light in a vacuum (km/s)
    speed_of_light=299792
    # Divide round-trip time by 2 to get one-way latency
    one_way_latency=$(echo "scale=6; $1 / 2.0" | bc)
    # Calculate distance using the formula: Distance = Speed of Light * Time
    printf "Formula: \nDistance = Speed of Light × Time \nTime = RTT/2 = %s \nSpeed of Light = %s km/s\n" "$one_way_latency" "$speed_of_light"
    distance_km=$(echo "scale=3; $speed_of_light * $one_way_latency / 1000.0" | bc)
    distance_miles=$(km_to_miles $distance_km)
    printf "Estimated distance: %.3f km or %s\n" "$distance_km" "$distance_miles"
}

# Define function to convert kilometers to miles
km_to_miles() {
    # 1 kilometer is approximately 0.621371 miles
    miles=$(echo "scale=3; $1 * 0.621371" | bc)
    printf "%.3f miles\n" "$miles"
}

# Define function to calculate real distance in kilometers and miles
calculate_real_distance() {
    # Purdue University coordinates (West Lafayette, Indiana, USA)
    purdue_lat=40.4259
    purdue_lon=-86.9081

    # Destination coordinates
    dest_lat=$1
    dest_lon=$2

    # Convert coordinates to radians
    deg2rad() {
        echo "scale=10; $1 * 0.0174533" | bc -l
    }
    purdue_lat=$(deg2rad $purdue_lat)
    purdue_lon=$(deg2rad $purdue_lon)
    dest_lat=$(deg2rad $dest_lat)
    dest_lon=$(deg2rad $dest_lon)

    # Calculate differences in latitude and longitude
    dlat=$(echo "$dest_lat - $purdue_lat" | bc -l)
    dlon=$(echo "$dest_lon - $purdue_lon" | bc -l)

    # Haversine formula for distance calculation
    a=$(echo "s($dlat/2)^2 + c($purdue_lat) * c($dest_lat) * s($dlon/2)^2" | bc -l)
    c=$(echo "2 * a( sqrt($a) )" | bc -l)
    radius_of_earth_km=6371  # Earth radius in km
    distance_km=$(echo "scale=3; $c * $radius_of_earth_km" | bc -l)
    # Convert distance to miles
    distance_miles=$(km_to_miles $distance_km)
    printf "Real distance from Purdue to destination: %.3f km or %s\n" "$distance_km" "$distance_miles"
}

# Function to ping a destination and calculate distances
ping_and_calculate_distances() {
    destination_name=$1
    latitude=$2
    longitude=$3
    echo "Pinging $destination_name ..."
    ping_result=$(ping -c 10 $destination_name | tail -n 1)
    avg_rtt=$(echo "$ping_result" | awk -F'/' '{print $5}')
    echo "$ping_result -> we will use avg_rtt = $avg_rtt"
    calculate_distance_km $avg_rtt
    calculate_real_distance $latitude $longitude
    echo
}

# Redirect output to a text file named "ping_results.txt"
{
    # Ping and calculate distances for each destination
    echo "Calculating distances for each destination..."
    ping_and_calculate_distances "www.mit.edu" 42.3601 -71.0942
    ping_and_calculate_distances "www.stanford.edu" 37.4275 -122.1697
    ping_and_calculate_distances "www.kyoto-u.ac.jp" 35.0116 135.7681
    ping_and_calculate_distances "www.cam.ac.uk" 52.2053 0.1218
    ping_and_calculate_distances "www.nus.edu.sg" 1.2956 103.7761
    echo "Distances calculation complete."

} | tee ping_results.txt
