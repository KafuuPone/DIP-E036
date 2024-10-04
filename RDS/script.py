def process_rds_file(input_filename):
  # Dictionary to store lines to be written to different files
  rds_data = {}

  with open(input_filename, 'r') as file:
    for line in file:
      # Check if the line starts with "[RDS]"
      if line.startswith("[RDS]"):
        # Split the line by spaces to extract the RDS number and the actual data
        parts = line.split(',', 2)  # Split the line at the first two commas
        
        # Get the RDS number (which comes right after "[RDS]")
        rds_number = parts[0].split()[1]
        
        # Get the data part, removing any leading/trailing whitespace
        data = parts[2].strip()

        # Add the line to the dictionary under the correct RDS number
        if rds_number not in rds_data:
          rds_data[rds_number] = []
        rds_data[rds_number].append(data)

  # Write each RDS number's data to a corresponding CSV file
  for rds_number, data_lines in rds_data.items():
    if len(data_lines) >= 20:
      output_filename = f"{rds_number}.csv"
      with open(output_filename, 'w') as outfile:
        for line in data_lines:
            outfile.write(line + '\n')

# Run the function with the input filename
process_rds_file('session.log')
