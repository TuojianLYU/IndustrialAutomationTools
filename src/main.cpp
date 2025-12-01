/**
 * @file main.cpp
 * @brief Advanced OPC UA Information Model - Manufacturing Cell Example
 *
 * This demonstrates the TRUE power of OPC UA:
 * 1. Type Inheritance (like OOP classes)
 * 2. Hierarchical Organization (Plant → Cell → Machine → Sensors)
 * 3. Methods for Control (Start/Stop machines)
 * 4. Multiple Data Types
 * 5. Machine States (Enum-like)
 *
 * Usage:
 *   ./opcua_server              - Use static code implementation
 *   ./opcua_server --xml <file> - Load information model from NodeSet XML
 *   ./opcua_server --help       - Show usage
 */

#include "open62541/plugin/log_stdout.h"
#include "open62541/server.h"
#include "open62541/server_config_default.h"

#ifdef UA_ENABLE_NODESETLOADER
#include "open62541/plugin/nodesetloader.h"
#endif

#include <cstring>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>

// ============================================================================
// MACHINE STATES (Simulating an Enum)
// ============================================================================
namespace MachineState {
const UA_Int32 IDLE = 0;
const UA_Int32 RUNNING = 1;
const UA_Int32 ERROR = 2;
const UA_Int32 MAINTENANCE = 3;
} // namespace MachineState

// ============================================================================
// GLOBAL
// ============================================================================
static volatile UA_Boolean running = true;
static void stopHandler(int sign) {
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Received Ctrl+C");
  running = false;
}

// Helper macro for cleaner code
#define UA_STR(s) const_cast<char *>(s)

// ============================================================================
// HELPER: Add a Variable to a Parent Node
// ============================================================================
UA_NodeId addVariable(UA_Server *server, UA_NodeId parentId, const char *name,
                      const char *description, void *value,
                      const UA_DataType *type,
                      UA_Byte accessLevel = UA_ACCESSLEVELMASK_READ) {
  UA_VariableAttributes attr = UA_VariableAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR(name));
  attr.description = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR(description));
  attr.accessLevel = accessLevel;
  UA_Variant_setScalar(&attr.value, value, type);

  UA_NodeId nodeId;
  UA_Server_addVariableNode(server, UA_NODEID_NULL, parentId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, UA_STR(name)),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            attr, NULL, &nodeId);
  return nodeId;
}

// ============================================================================
// HELPER: Add Variable to Type (with Modelling Rule = Mandatory)
// ============================================================================
void addTypeVariable(UA_Server *server, UA_NodeId typeId, const char *name,
                     const char *description, void *value,
                     const UA_DataType *type,
                     UA_Byte accessLevel = UA_ACCESSLEVELMASK_READ |
                                           UA_ACCESSLEVELMASK_WRITE) {
  UA_VariableAttributes attr = UA_VariableAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR(name));
  attr.description = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR(description));
  attr.accessLevel = accessLevel;
  UA_Variant_setScalar(&attr.value, value, type);

  UA_NodeId varId;
  UA_Server_addVariableNode(server, UA_NODEID_NULL, typeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, UA_STR(name)),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            attr, NULL, &varId);

  // Mark as Mandatory - instances will auto-create this variable
  UA_Server_addReference(
      server, varId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}

// ============================================================================
// METHOD CALLBACKS
// ============================================================================
static UA_StatusCode startMachineCallback(
    UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
    const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
    void *objectContext, size_t inputSize, const UA_Variant *input,
    size_t outputSize, UA_Variant *output) {
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "🟢 START method called!");

  // In real implementation: Find the State variable and set it to RUNNING
  // For demo, just log
  return UA_STATUSCODE_GOOD;
}

static UA_StatusCode stopMachineCallback(
    UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
    const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
    void *objectContext, size_t inputSize, const UA_Variant *input,
    size_t outputSize, UA_Variant *output) {
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "🔴 STOP method called!");
  return UA_STATUSCODE_GOOD;
}

// ============================================================================
// 1. BASE TYPE: MachineType (Abstract base for all machines)
// ============================================================================
UA_NodeId defineMachineType(UA_Server *server) {
  UA_NodeId machineTypeId = UA_NODEID_STRING(1, UA_STR("MachineType"));

  UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("MachineType"));
  attr.description = UA_LOCALIZEDTEXT(
      UA_STR("en-US"), UA_STR("Base type for all industrial machines"));
  attr.isAbstract = true; // Cannot instantiate directly!

  UA_Server_addObjectTypeNode(
      server, machineTypeId, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
      UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
      UA_QUALIFIEDNAME(1, UA_STR("MachineType")), attr, NULL, NULL);

  // Common properties ALL machines have:
  UA_Int32 state = MachineState::IDLE;
  addTypeVariable(server, machineTypeId, "State",
                  "Machine state: 0=Idle, 1=Running, 2=Error, 3=Maintenance",
                  &state, &UA_TYPES[UA_TYPES_INT32]);

  UA_Boolean alarm = false;
  addTypeVariable(server, machineTypeId, "AlarmActive",
                  "True if machine has an active alarm", &alarm,
                  &UA_TYPES[UA_TYPES_BOOLEAN], UA_ACCESSLEVELMASK_READ);

  UA_Double hours = 0.0;
  addTypeVariable(server, machineTypeId, "OperatingHours",
                  "Total operating hours", &hours, &UA_TYPES[UA_TYPES_DOUBLE],
                  UA_ACCESSLEVELMASK_READ);

  UA_String serial = UA_STRING(UA_STR("N/A"));
  addTypeVariable(server, machineTypeId, "SerialNumber",
                  "Machine serial number", &serial, &UA_TYPES[UA_TYPES_STRING],
                  UA_ACCESSLEVELMASK_READ);

  std::cout << "✅ MachineType (abstract base) defined" << std::endl;
  return machineTypeId;
}

// ============================================================================
// 2. CNC MACHINE TYPE (Inherits from MachineType)
// ============================================================================
UA_NodeId defineCNCMachineType(UA_Server *server, UA_NodeId parentTypeId) {
  UA_NodeId cncTypeId = UA_NODEID_STRING(1, UA_STR("CNCMachineType"));

  UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
  attr.displayName =
      UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("CNCMachineType"));
  attr.description =
      UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("CNC Milling/Turning Machine"));

  // INHERIT from MachineType (not BaseObjectType!)
  UA_Server_addObjectTypeNode(server, cncTypeId, parentTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                              UA_QUALIFIEDNAME(1, UA_STR("CNCMachineType")),
                              attr, NULL, NULL);

  // CNC-specific properties:
  UA_Double spindleSpeed = 0.0;
  addTypeVariable(server, cncTypeId, "SpindleSpeed",
                  "Spindle rotation speed (RPM)", &spindleSpeed,
                  &UA_TYPES[UA_TYPES_DOUBLE]);

  UA_Double feedRate = 0.0;
  addTypeVariable(server, cncTypeId, "FeedRate", "Tool feed rate (mm/min)",
                  &feedRate, &UA_TYPES[UA_TYPES_DOUBLE]);

  UA_Int32 toolNumber = 1;
  addTypeVariable(server, cncTypeId, "CurrentTool",
                  "Currently loaded tool number", &toolNumber,
                  &UA_TYPES[UA_TYPES_INT32]);

  UA_String program = UA_STRING(UA_STR("None"));
  addTypeVariable(server, cncTypeId, "ActiveProgram",
                  "Currently running NC program", &program,
                  &UA_TYPES[UA_TYPES_STRING], UA_ACCESSLEVELMASK_READ);

  std::cout << "✅ CNCMachineType defined (inherits MachineType)" << std::endl;
  return cncTypeId;
}

// ============================================================================
// 3. ROBOT ARM TYPE (Inherits from MachineType)
// ============================================================================
UA_NodeId defineRobotArmType(UA_Server *server, UA_NodeId parentTypeId) {
  UA_NodeId robotTypeId = UA_NODEID_STRING(1, UA_STR("RobotArmType"));

  UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("RobotArmType"));
  attr.description =
      UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("6-Axis Industrial Robot Arm"));

  UA_Server_addObjectTypeNode(server, robotTypeId, parentTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                              UA_QUALIFIEDNAME(1, UA_STR("RobotArmType")), attr,
                              NULL, NULL);

  // Robot-specific properties:
  UA_Double posX = 0.0, posY = 0.0, posZ = 0.0;
  addTypeVariable(server, robotTypeId, "PositionX", "TCP X position (mm)",
                  &posX, &UA_TYPES[UA_TYPES_DOUBLE], UA_ACCESSLEVELMASK_READ);
  addTypeVariable(server, robotTypeId, "PositionY", "TCP Y position (mm)",
                  &posY, &UA_TYPES[UA_TYPES_DOUBLE], UA_ACCESSLEVELMASK_READ);
  addTypeVariable(server, robotTypeId, "PositionZ", "TCP Z position (mm)",
                  &posZ, &UA_TYPES[UA_TYPES_DOUBLE], UA_ACCESSLEVELMASK_READ);

  UA_Boolean gripperClosed = false;
  addTypeVariable(server, robotTypeId, "GripperClosed",
                  "True if gripper is closed", &gripperClosed,
                  &UA_TYPES[UA_TYPES_BOOLEAN]);

  UA_Double speed = 50.0;
  addTypeVariable(server, robotTypeId, "SpeedPercent",
                  "Robot speed override (0-100%)", &speed,
                  &UA_TYPES[UA_TYPES_DOUBLE]);

  std::cout << "✅ RobotArmType defined (inherits MachineType)" << std::endl;
  return robotTypeId;
}

// ============================================================================
// 4. CONVEYOR TYPE (Inherits from MachineType)
// ============================================================================
UA_NodeId defineConveyorType(UA_Server *server, UA_NodeId parentTypeId) {
  UA_NodeId conveyorTypeId = UA_NODEID_STRING(1, UA_STR("ConveyorType"));

  UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("ConveyorType"));
  attr.description =
      UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("Industrial Belt Conveyor"));

  UA_Server_addObjectTypeNode(server, conveyorTypeId, parentTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                              UA_QUALIFIEDNAME(1, UA_STR("ConveyorType")), attr,
                              NULL, NULL);

  // Conveyor-specific properties:
  UA_Double speed = 0.0;
  addTypeVariable(server, conveyorTypeId, "BeltSpeed", "Belt speed (m/min)",
                  &speed, &UA_TYPES[UA_TYPES_DOUBLE]);

  UA_Boolean direction = true; // true = forward
  addTypeVariable(server, conveyorTypeId, "DirectionForward",
                  "True = Forward, False = Reverse", &direction,
                  &UA_TYPES[UA_TYPES_BOOLEAN]);

  UA_Boolean sensorEntry = false;
  addTypeVariable(server, conveyorTypeId, "SensorEntry",
                  "Part detected at entry", &sensorEntry,
                  &UA_TYPES[UA_TYPES_BOOLEAN], UA_ACCESSLEVELMASK_READ);

  UA_Boolean sensorExit = false;
  addTypeVariable(server, conveyorTypeId, "SensorExit", "Part detected at exit",
                  &sensorExit, &UA_TYPES[UA_TYPES_BOOLEAN],
                  UA_ACCESSLEVELMASK_READ);

  std::cout << "✅ ConveyorType defined (inherits MachineType)" << std::endl;
  return conveyorTypeId;
}

// ============================================================================
// 5. CREATE MANUFACTURING CELL (Container Folder)
// ============================================================================
UA_NodeId createManufacturingCell(UA_Server *server, const char *cellName) {
  UA_ObjectAttributes attr = UA_ObjectAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR(cellName));
  attr.description = UA_LOCALIZEDTEXT(
      UA_STR("en-US"), UA_STR("Manufacturing Cell containing machines"));

  UA_NodeId cellId;
  UA_Server_addObjectNode(
      server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
      UA_QUALIFIEDNAME(1, UA_STR(cellName)),
      UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), attr, NULL, &cellId);

  std::cout << "📦 Created Manufacturing Cell: " << cellName << std::endl;
  return cellId;
}

// ============================================================================
// 6. INSTANTIATE MACHINES
// ============================================================================
UA_NodeId createMachineInstance(UA_Server *server, UA_NodeId parentId,
                                UA_NodeId typeId, const char *name) {
  UA_ObjectAttributes attr = UA_ObjectAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR(name));

  UA_NodeId instanceId;
  UA_Server_addObjectNode(
      server, UA_NODEID_NULL, parentId,
      UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
      UA_QUALIFIEDNAME(1, UA_STR(name)),
      typeId, // <-- This is the magic: inherits all properties!
      attr, NULL, &instanceId);

  // Add Start/Stop methods to this instance
  UA_MethodAttributes startAttr = UA_MethodAttributes_default;
  startAttr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("Start"));
  startAttr.executable = true;
  startAttr.userExecutable = true;
  UA_Server_addMethodNode(server, UA_NODEID_NULL, instanceId,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          UA_QUALIFIEDNAME(1, UA_STR("Start")), startAttr,
                          &startMachineCallback, 0, NULL, 0, NULL, NULL, NULL);

  UA_MethodAttributes stopAttr = UA_MethodAttributes_default;
  stopAttr.displayName = UA_LOCALIZEDTEXT(UA_STR("en-US"), UA_STR("Stop"));
  stopAttr.executable = true;
  stopAttr.userExecutable = true;
  UA_Server_addMethodNode(server, UA_NODEID_NULL, instanceId,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                          UA_QUALIFIEDNAME(1, UA_STR("Stop")), stopAttr,
                          &stopMachineCallback, 0, NULL, 0, NULL, NULL, NULL);

  std::cout << "  🔧 Created machine instance: " << name << std::endl;
  return instanceId;
}

// ============================================================================
// STATIC MODEL BUILDER (Original implementation)
// ============================================================================
void buildStaticModel(UA_Server *server) {
  // =========================================
  // PHASE 1: Define the Information Model (Types)
  // =========================================
  std::cout << "📐 Phase 1: Defining Type Hierarchy...\n" << std::endl;

  UA_NodeId machineType = defineMachineType(server);
  UA_NodeId cncType = defineCNCMachineType(server, machineType);
  UA_NodeId robotType = defineRobotArmType(server, machineType);
  UA_NodeId conveyorType = defineConveyorType(server, machineType);

  // =========================================
  // PHASE 2: Create the Plant Structure
  // =========================================
  std::cout << "\n🏭 Phase 2: Creating Plant Structure...\n" << std::endl;

  // Create Cell 1: CNC Machining Cell
  UA_NodeId cell1 = createManufacturingCell(server, "Cell_CNC_Machining");
  createMachineInstance(server, cell1, cncType, "CNC_Lathe_01");
  createMachineInstance(server, cell1, cncType, "CNC_Mill_01");
  createMachineInstance(server, cell1, robotType, "LoadingRobot_01");
  createMachineInstance(server, cell1, conveyorType, "PartConveyor_01");

  // Create Cell 2: Assembly Cell
  UA_NodeId cell2 = createManufacturingCell(server, "Cell_Assembly");
  createMachineInstance(server, cell2, robotType, "AssemblyRobot_01");
  createMachineInstance(server, cell2, robotType, "AssemblyRobot_02");
  createMachineInstance(server, cell2, conveyorType, "AssemblyLine_01");
}

// ============================================================================
// XML NODESET LOADER
// ============================================================================
bool loadNodesetFromXml(UA_Server *server, const std::string &xmlPath) {
  // Check if file exists
  std::ifstream file(xmlPath);
  if (!file.good()) {
    std::cerr << "❌ Error: Cannot open XML file: " << xmlPath << std::endl;
    return false;
  }
  file.close();

#ifdef UA_ENABLE_NODESETLOADER
  std::cout << "📄 Loading NodeSet from XML: " << xmlPath << std::endl;

  UA_StatusCode retval = UA_Server_loadNodeset(server, xmlPath.c_str(), NULL);
  if (retval != UA_STATUSCODE_GOOD) {
    std::cerr << "❌ Error loading nodeset: " << UA_StatusCode_name(retval)
              << std::endl;
    return false;
  }

  std::cout << "✅ NodeSet loaded successfully from XML!" << std::endl;
  return true;
#else
  std::cerr << "❌ Error: NodeSet loader not enabled!" << std::endl;
  std::cerr << "   Rebuild open62541 with UA_ENABLE_NODESETLOADER=ON"
            << std::endl;
  std::cerr << "   Or use the static code implementation instead." << std::endl;
  return false;
#endif
}

// ============================================================================
// USAGE HELP
// ============================================================================
void printUsage(const char *progName) {
  std::cout << "\nUsage: " << progName << " [OPTIONS]\n" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  (no args)         Use static code implementation (default)"
            << std::endl;
  std::cout
      << "  --xml <file>      Load information model from NodeSet XML file"
      << std::endl;
  std::cout << "  --help, -h        Show this help message\n" << std::endl;
  std::cout << "Examples:" << std::endl;
  std::cout << "  " << progName << std::endl;
  std::cout << "  " << progName << " --xml /path/to/NodeSet.xml\n" << std::endl;
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char *argv[]) {
  signal(SIGINT, stopHandler);
  signal(SIGTERM, stopHandler);

  // Parse command-line arguments
  std::string xmlPath;
  bool useXml = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUsage(argv[0]);
      return EXIT_SUCCESS;
    } else if (arg == "--xml") {
      if (i + 1 < argc) {
        xmlPath = argv[++i];
        useXml = true;
      } else {
        std::cerr << "❌ Error: --xml requires a file path argument"
                  << std::endl;
        printUsage(argv[0]);
        return EXIT_FAILURE;
      }
    } else {
      std::cerr << "❌ Unknown option: " << arg << std::endl;
      printUsage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  // Create server
  UA_Server *server = UA_Server_new();
  UA_ServerConfig *config = UA_Server_getConfig(server);
  UA_ServerConfig_setMinimal(config, 4840, NULL);

  std::cout << "\n========================================" << std::endl;
  std::cout << "  OPC UA Manufacturing Cell Demo" << std::endl;
  std::cout << "========================================\n" << std::endl;

  // Build information model based on mode
  if (useXml) {
    std::cout << "🔧 Mode: XML NodeSet Loading\n" << std::endl;
    if (!loadNodesetFromXml(server, xmlPath)) {
      UA_Server_delete(server);
      return EXIT_FAILURE;
    }
  } else {
    std::cout << "🔧 Mode: Static Code Implementation\n" << std::endl;
    buildStaticModel(server);
  }

  // =========================================
  // Run Server
  // =========================================
  std::cout << "\n========================================" << std::endl;
  std::cout << "🚀 Server running at opc.tcp://localhost:4840" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "\n📊 Browse the address space to see:" << std::endl;
  std::cout << "   - Type hierarchy under 'Types → ObjectTypes'" << std::endl;
  std::cout << "   - Instances under 'Objects → Cell_*'" << std::endl;
  if (!useXml) {
    std::cout << "   - Try calling Start/Stop methods!" << std::endl;
  }
  std::cout << std::endl;

  UA_StatusCode retval = UA_Server_run(server, &running);

  UA_Server_delete(server);
  return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
