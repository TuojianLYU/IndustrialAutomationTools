#include <errno.h>
#include <iostream>
#include <modbus.h>
#include <unistd.h>

using namespace std;

int main() {
  modbus_t *ctx = modbus_new_tcp("127.0.0.1", 1502);

  if (!ctx) {
    cerr << "Failed to create context." << endl;
    return -1;
  }

  modbus_set_debug(ctx, TRUE);

  // Create mapping: 10 coils, 10 discrete inputs, 10 holding regs, 10 input
  // regs
  modbus_mapping_t *mb_mapping = modbus_mapping_new(10, 10, 10, 10);

  if (!mb_mapping) {
    cerr << "Failed to allocate memory mapping." << endl;
    modbus_free(ctx);
    return -1;
  }

  // =========================================================================
  // Initialize all 4 data types with test values
  // =========================================================================

  // 1. Coils (tab_bits) - Read/Write bits - FC01, FC05, FC15
  mb_mapping->tab_bits[0] = 1; // ON
  mb_mapping->tab_bits[1] = 0; // OFF
  mb_mapping->tab_bits[2] = 1; // ON
  mb_mapping->tab_bits[3] = 1; // ON
  mb_mapping->tab_bits[4] = 0; // OFF
  cout << "📝 Coils initialized: [1,0,1,1,0,0,0,0,0,0]" << endl;

  // 2. Discrete Inputs (tab_input_bits) - Read-only bits - FC02
  mb_mapping->tab_input_bits[0] = 1;
  mb_mapping->tab_input_bits[1] = 1;
  mb_mapping->tab_input_bits[2] = 0;
  mb_mapping->tab_input_bits[3] = 1;
  mb_mapping->tab_input_bits[4] = 0;
  cout << "📝 Discrete Inputs initialized: [1,1,0,1,0,0,0,0,0,0]" << endl;

  // 3. Holding Registers (tab_registers) - Read/Write 16-bit - FC03, FC06, FC16
  mb_mapping->tab_registers[0] = 100;
  mb_mapping->tab_registers[1] = 200;
  mb_mapping->tab_registers[2] = 300;
  mb_mapping->tab_registers[3] = 400;
  mb_mapping->tab_registers[4] = 500;
  cout << "📝 Holding Registers initialized: [100,200,300,400,500,...]" << endl;

  // 4. Input Registers (tab_input_registers) - Read-only 16-bit - FC04
  mb_mapping->tab_input_registers[0] = 1111;
  mb_mapping->tab_input_registers[1] = 2222;
  mb_mapping->tab_input_registers[2] = 3333;
  mb_mapping->tab_input_registers[3] = 4444;
  mb_mapping->tab_input_registers[4] = 5555;
  cout << "📝 Input Registers initialized: [1111,2222,3333,4444,5555,...]"
       << endl;

  // =========================================================================
  // Start server
  // =========================================================================
  int server_socket = modbus_tcp_listen(ctx, 1);
  if (server_socket == -1) {
    cerr << "Unable to listen: " << modbus_strerror(errno) << endl;
    modbus_mapping_free(mb_mapping);
    modbus_free(ctx);
    return -1;
  }

  cout << "\n========================================" << endl;
  cout << "🚀 Modbus TCP Server listening on port 1502" << endl;
  cout << "========================================" << endl;
  cout << "Waiting for client connection..." << endl;

  int client_socket = modbus_tcp_accept(ctx, &server_socket);
  if (client_socket == -1) {
    cerr << "Unable to accept client: " << modbus_strerror(errno) << endl;
    close(server_socket);
    modbus_mapping_free(mb_mapping);
    modbus_free(ctx);
    return -1;
  }

  cout << "✅ Client connected!\n" << endl;

  uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

  while (true) {
    int rc = modbus_receive(ctx, query);

    if (rc > 0) {
      // Automatically handles all function codes based on the query
      modbus_reply(ctx, query, rc, mb_mapping);
    } else if (rc == -1) {
      cout << "\n❌ Connection closed." << endl;
      break;
    }
  }

  cout << "🛑 Stopping server." << endl;
  modbus_mapping_free(mb_mapping);
  close(server_socket);
  modbus_free(ctx);

  return 0;
}
