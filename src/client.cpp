#include <errno.h>
#include <iostream>
#include <modbus.h>

using namespace std;

void printBits(const char *label, uint8_t *bits, int count) {
  cout << label;
  for (int i = 0; i < count; i++) {
    cout << "[" << i << "]:" << (bits[i] ? "1" : "0") << " ";
  }
  cout << endl;
}

void printRegisters(const char *label, uint16_t *regs, int count) {
  cout << label;
  for (int i = 0; i < count; i++) {
    cout << "[" << i << "]:" << regs[i] << " ";
  }
  cout << endl;
}

int main() {
  modbus_t *ctx = modbus_new_tcp("127.0.0.1", 1502);

  if (!ctx) {
    cerr << "Failed to create context." << endl;
    return -1;
  }

  if (modbus_connect(ctx) == -1) {
    cerr << "Connection failed: " << modbus_strerror(errno) << endl;
    modbus_free(ctx);
    return -1;
  }
  cout << "✅ Connected to server.\n" << endl;

  uint8_t bit_buffer[10] = {0,0,0,0,0,0,0,0,0,0};
  uint16_t reg_buffer[10] = {0,0,0,0,0,0,0,0,0,0};
  int rc;

  // =========================================================================
  // FC01: Read Coils (Read/Write bits)
  // =========================================================================
  cout << "========== FC01: READ COILS ==========" << endl;
  rc = modbus_read_bits(ctx, 0, 5, bit_buffer);
  if (rc != -1) {
    printBits("📥 Read Coils: ", bit_buffer, rc);
  } else {
    cerr << "❌ FC01 failed: " << modbus_strerror(errno) << endl;
  }

  // =========================================================================
  // FC02: Read Discrete Inputs (Read-only bits)
  // =========================================================================
  cout << "\n========== FC02: READ DISCRETE INPUTS ==========" << endl;
  rc = modbus_read_input_bits(ctx, 0, 5, bit_buffer);
  if (rc != -1) {
    printBits("📥 Read Discrete Inputs: ", bit_buffer, rc);
  } else {
    cerr << "❌ FC02 failed: " << modbus_strerror(errno) << endl;
  }

  // =========================================================================
  // FC03: Read Holding Registers (Read/Write 16-bit)
  // =========================================================================
  cout << "\n========== FC03: READ HOLDING REGISTERS ==========" << endl;
  rc = modbus_read_registers(ctx, 0, 5, reg_buffer);
  if (rc != -1) {
    printRegisters("📥 Read Holding Registers: ", reg_buffer, rc);
  } else {
    cerr << "❌ FC03 failed: " << modbus_strerror(errno) << endl;
  }

  // =========================================================================
  // FC04: Read Input Registers (Read-only 16-bit)
  // =========================================================================
  cout << "\n========== FC04: READ INPUT REGISTERS ==========" << endl;
  rc = modbus_read_input_registers(ctx, 0, 5, reg_buffer);
  if (rc != -1) {
    printRegisters("📥 Read Input Registers: ", reg_buffer, rc);
  } else {
    cerr << "❌ FC04 failed: " << modbus_strerror(errno) << endl;
  }

  // =========================================================================
  // FC05: Write Single Coil
  // =========================================================================
  cout << "\n========== FC05: WRITE SINGLE COIL ==========" << endl;
  rc = modbus_write_bit(ctx, 0, FALSE); // Turn OFF coil[0]
  if (rc != -1) {
    cout << "📤 Wrote Coil[0] = OFF (was ON)" << endl;
  } else {
    cerr << "❌ FC05 failed: " << modbus_strerror(errno) << endl;
  }

  // Verify the write
  rc = modbus_read_bits(ctx, 0, 5, bit_buffer);
  if (rc != -1) {
    printBits("📥 Verify Coils: ", bit_buffer, rc);
  }

  // =========================================================================
  // FC06: Write Single Register
  // =========================================================================
  cout << "\n========== FC06: WRITE SINGLE REGISTER ==========" << endl;
  rc = modbus_write_register(ctx, 0, 9999); // Write 9999 to register[0]
  if (rc != -1) {
    cout << "📤 Wrote Holding Register[0] = 9999 (was 100)" << endl;
  } else {
    cerr << "❌ FC06 failed: " << modbus_strerror(errno) << endl;
  }

  // Verify the write
  rc = modbus_read_registers(ctx, 0, 5, reg_buffer);
  if (rc != -1) {
    printRegisters("📥 Verify Holding Registers: ", reg_buffer, rc);
  }

  // =========================================================================
  // FC15: Write Multiple Coils
  // =========================================================================
  cout << "\n========== FC15: WRITE MULTIPLE COILS ==========" << endl;
  uint8_t coils_to_write[5] = {1, 1, 1, 1, 1}; // Set all 5 to ON
  rc = modbus_write_bits(ctx, 0, 5, coils_to_write);
  if (rc != -1) {
    cout << "📤 Wrote Coils[0-4] = [1,1,1,1,1] (all ON)" << endl;
  } else {
    cerr << "❌ FC15 failed: " << modbus_strerror(errno) << endl;
  }

  // Verify the write
  rc = modbus_read_bits(ctx, 0, 5, bit_buffer);
  if (rc != -1) {
    printBits("📥 Verify Coils: ", bit_buffer, rc);
  }

  // =========================================================================
  // FC16: Write Multiple Registers
  // =========================================================================
  cout << "\n========== FC16: WRITE MULTIPLE REGISTERS ==========" << endl;
  uint16_t regs_to_write[3] = {1000, 2000, 3000};
  rc = modbus_write_registers(ctx, 2, 3, regs_to_write); // Write to reg[2-4]
  if (rc != -1) {
    cout << "📤 Wrote Holding Registers[2-4] = [1000,2000,3000]" << endl;
  } else {
    cerr << "❌ FC16 failed: " << modbus_strerror(errno) << endl;
  }

  // Verify the write
  rc = modbus_read_registers(ctx, 0, 5, reg_buffer);
  if (rc != -1) {
    printRegisters("📥 Verify Holding Registers: ", reg_buffer, rc);
  }

  // =========================================================================
  // FC23: Read/Write Multiple Registers (if supported)
  // =========================================================================
  cout << "\n========== FC23: READ/WRITE MULTIPLE REGISTERS ==========" << endl;
  uint16_t write_regs[2] = {7777, 8888};
  uint16_t read_regs[3];
  rc = modbus_write_and_read_registers(ctx, 0, 2,
                                       write_regs,       // Write to reg[0-1]
                                       2, 3, read_regs); // Read from reg[2-4]
  if (rc != -1) {
    cout << "📤 Wrote Registers[0-1] = [7777,8888]" << endl;
    printRegisters("📥 Read Registers[2-4]: ", read_regs, rc);
  } else {
    cerr << "❌ FC23 failed: " << modbus_strerror(errno) << endl;
  }

  // Final state
  cout << "\n========== FINAL STATE ==========" << endl;
  modbus_read_bits(ctx, 0, 5, bit_buffer);
  printBits("Coils: ", bit_buffer, 5);

  modbus_read_registers(ctx, 0, 5, reg_buffer);
  printRegisters("Holding Registers: ", reg_buffer, 5);

  // =========================================================================
  // Cleanup
  // =========================================================================
  cout << "\n🛑 Closing connection." << endl;
  modbus_close(ctx);
  modbus_free(ctx);

  return 0;
}
