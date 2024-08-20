# Create a binary file with a specific byte
file_path = r"C:\Users\mavo1\Desktop\specific_byte.bin"
specific_byte = bytes([0x55])

# Write the specific byte to the file
with open(file_path, "wb") as file:
    file.write(specific_byte)
