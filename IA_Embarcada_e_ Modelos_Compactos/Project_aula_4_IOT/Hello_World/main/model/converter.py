with open("hello_world_int8.tflite", "rb") as f:
    bytes_data = f.read()

with open("hello_world_int8.h", "w") as f:
    f.write("#ifndef HELLO_WORLD_INT8_H\n#define HELLO_WORLD_INT8_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write("alignas(16) const unsigned char g_hello_world_int8[] = {\n")
    
    line = []
    for byte in bytes_data:
        line.append(f"0x{byte:02x}")
        if len(line) == 12:
            f.write("    " + ", ".join(line) + ",\n")
            line = []
    if line:
        f.write("    " + ", ".join(line) + "\n")
        
    f.write("};\n\n")
    f.write(f"const unsigned int g_hello_world_int8_len = {len(bytes_data)};\n\n")
    f.write("#endif // HELLO_WORLD_INT8_H\n")

print(f"Sucesso! Gerado hello_world_int8.h ({len(bytes_data)} bytes)")