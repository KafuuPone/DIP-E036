def binary_to_ascii(binary_str):
  # Convert binary string to an integer representing the ASCII value
  ascii_value = int(binary_str, 2)
    
  # Check if the ASCII value is between 32 (space) and 126 (~)
  if 32 <= ascii_value <= 126:
    return chr(ascii_value)
  else:
    # Return 'NUL' for characters outside the range
    return '\x00'  # NUL symbol

def process_csv(input_filename, output_filename):
  with open(input_filename, 'r') as infile, open(output_filename, 'w') as outfile:
    for line in infile:
      # Split the line by commas to get the binary values
      parts = [part.strip() for part in line.strip().split(',') if part.strip()]  # Filter out empty cells

      # Get the last 4 non-empty binary bytes
      last_four_bytes = parts[-4:]

      # Convert each binary byte to ASCII, replacing out-of-range chars with NUL
      ascii_chars = [binary_to_ascii(byte) for byte in last_four_bytes]

      # Join the ASCII characters into a string
      ascii_string = ''.join(ascii_chars)

      # Write the original line followed by the ASCII characters to the output file
      outfile.write(line.strip() + ' -> ' + ascii_string + '\n')

# Run the function with the input and output filenames
process_csv('958.csv', '958_ascii.txt')