./config.sh champsim_config.json
make
#!/bin/bash

# Create the results directory if it doesn't exist
mkdir -p results_spec

# Loop through each file in the traces directory
for trace_file in ../../../spec/*; do
    # Extract the filename without the directory path
    trace_name=$(basename "$trace_file")
    
    echo "Running ${trace_name}"
    # Run champsim and redirect the output to results directory
    ./bin/champsim --warmup-instructions 25000000 --simulation-instructions 25000000 "$trace_file" > "results_spec/${trace_name}.out"
    
    echo "Simulation for $trace_name completed and saved to results/${trace_name}.out"
done

