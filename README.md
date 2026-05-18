# Agentino

**Agentino** is a distributed service management system that enables centralized monitoring, control, and administration of services across multiple agents in a network. Built with C++ and Qt framework, it provides a modern GraphQL-based API for real-time service lifecycle management.

## Overview

Agentino implements an agent-based architecture where a central server manages multiple agent nodes, each responsible for running and monitoring services on their respective machines. The system provides real-time status updates, service discovery, dependency management, and comprehensive logging capabilities through a modern GraphQL interface.

### Key Features

- **🔄 Distributed Service Management** - Start, stop, configure, and monitor services remotely across multiple agents
- **📊 Real-time Monitoring** - Live status updates and health tracking via GraphQL subscriptions
- **🌐 Multi-Agent Topology** - Manage interconnected agent networks with visual topology representation
- **🔍 Service Discovery** - Automatic detection and registration of services across the network
- **📝 Comprehensive Logging** - Centralized log collection and viewing from all managed services
- **🔗 Dependency Management** - Track and manage service dependencies and execution order
- **🎨 Modern UI** - Qt Quick/QML-based graphical interface with responsive design
- **🔌 GraphQL API** - Flexible query and subscription interface for integration with other systems

## Architecture

Agentino follows a layered component architecture:

```
┌─────────────────────────────────────────────────────┐
│              UI Layer (Qt Quick/QML)                │
│  AgentinoServer (GUI) | AgentinoClient              │
└─────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────┐
│            GraphQL API Layer                        │
│  agentinogql (server) | agentgql (client)          │
│         AgentinoGqlPck (schema)                     │
└─────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────┐
│       Data & Business Logic Layer                   │
│  agentinodata (models) | AgentinoDataPck            │
│  AgentinoLoc (licensing/localization)               │
└─────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────┐
│          Communication Layer                         │
│  AgentinoClientServer | ImtCore frameworks          │
└─────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────┐
│             Service Layer                            │
│  AgentinoAgent (remote) | AgentinoServer (central)  │
└─────────────────────────────────────────────────────┘
```

## Project Structure

```
Agentino/
├── Include/              # Header files for all modules
│   ├── agentino/        # Core definitions and version info
│   ├── agentinodata/    # Data models (Service/Agent info, controllers)
│   ├── agentinogql/     # Server-side GraphQL resolvers
│   ├── agentgql/        # Client-side GraphQL integration
│   └── agentinoqml/     # QML/UI bindings for Qt Quick
│
├── Impl/                 # Implementation files
│   ├── AgentinoServer/  # Main server application
│   ├── AgentinoClient/  # Client application
│   ├── AgentinoAgent/   # Agent service for managed nodes
│   ├── AgentinoDataPck/ # Data package with component factories
│   ├── AgentinoGqlPck/  # GraphQL schema implementation
│   ├── AgentGqlPck/     # Client GraphQL package
│   ├── AgentinoLoc/     # Localization and licensing
│   └── AgentinoClientServer/ # Shared client-server communication
│
├── Build/               # Build configurations
│   ├── CMake/          # CMake build files (primary)
│   ├── QMake/          # QMake project files (alternative)
│   └── VC*/            # Visual Studio configurations
│
├── Config/              # Configuration files
│   ├── CMake/          # CMake environment configuration
│   └── *.awc           # ARX compiler configurations
│
├── Docs/                # Documentation
│   ├── Doxyfile        # Doxygen configuration
│   └── GenerateDocs.bat # Documentation generation script
│
├── Install/             # Installation resources
│   ├── AgentinoAgent/  # Agent installer scripts
│   ├── AgentinoClient/ # Client installer scripts
│   ├── AgentinoServer/ # Server installer scripts
│   ├── Commercial/     # Commercial license files
│   └── LGPL/           # Open-source license files
│
├── Sdl/                 # Service Definition Language files
└── Partitura/           # Additional resources
```

## Main Components

| Component | Type | Description |
|-----------|------|-------------|
| **agentinodata** | Library | Core data models for services, agents, and status information |
| **agentinogql** | Library | Server-side GraphQL resolvers and subscriptions |
| **agentgql** | Library | Client-side GraphQL integration and queries |
| **agentinoqml** | Library | QML bindings for UI components |
| **AgentinoServer** | Executable | Central server application with GUI |
| **AgentinoClient** | Executable | Client application for service management |
| **AgentinoAgent** | Executable | Agent service that runs on managed nodes |
| **AgentinoGqlPck** | Package | GraphQL schema and type definitions |
| **AgentGqlPck** | Package | Client-side GraphQL package |
| **AgentinoDataPck** | Package | Component factories and data utilities |
| **AgentinoLoc** | Library | Localization and licensing functionality |

## Requirements

### System Requirements

- **Operating System**: Windows, macOS, or Linux
- **Compiler**: 
  - Windows: Visual Studio 2017, 2019, or 2022 (MSVC)
  - Linux/macOS: GCC or Clang with C++17 support
- **CMake**: Version 3.26 or higher
- **Qt Framework**: Qt 5.x or Qt 6.x

### Dependencies

#### Core Dependencies
- **Qt 5/6** - Required components:
  - Core, Widgets, QML, Quick, QuickWidgets
  - Network, WebSockets, SQL, Xml, Svg
  - Concurrent, QuickControls2, PrintSupport
  - Core5Compat (Qt6 only)

- **ImtCore Framework** - Enterprise application framework providing:
  - `imtservice` - Service management foundation
  - `imtgql` - GraphQL implementation
  - `imtauth` - Authentication and authorization
  - `imtdb` - Database abstraction
  - `imtmail` - Email functionality
  - `imtbase`, `imtcom`, `imtapp` - Core utilities
  - `imtlog`, `imtfile` - Logging and file operations
  - `imtstyle`, `imtgui`, `imtqml` - UI components

- **ACF** - Application Component Framework for component architecture and licensing

#### External Libraries
- **OpenSSL** - Security and encryption
- **Quazip** - ZIP compression/decompression
- **Doxygen** - Documentation generation (optional)

## Building from Source

### Prerequisites

1. **Set Environment Variables**:
   ```bash
   # ImtCore directory
   export IMTCOREDIR=/path/to/imtcore
   
   # Qt directory (if not auto-detected)
   export QTDIR=/path/to/qt
   
   # Service Manager directory (optional)
   export SMDIR=/path/to/sm
   ```

2. **Install Qt**:
   - Download and install Qt 5.15+ or Qt 6.x from [qt.io](https://www.qt.io/download)
   - Ensure Qt is in your PATH or set QTDIR

3. **Install ImtCore Framework**:
   - Follow ImtCore installation instructions
   - Set IMTCOREDIR environment variable

### Build with CMake (Recommended)

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ../Build/CMake \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_VERSION_MAJOR=6 \
    -DWEB_COMPILE=ON

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --prefix /path/to/install
```

### Build with QMake (Alternative)

```bash
cd Build/QMake

# Generate Makefiles
qmake AgentinoAll.pro

# Build
make

# Or on Windows with nmake
nmake
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `QT_VERSION_MAJOR` | Auto-detect | Specify Qt version (5 or 6) |
| `WEB_COMPILE` | ON | Enable web compatibility features |
| `CMAKE_BUILD_TYPE` | Debug | Build type (Debug/Release/RelWithDebInfo) |
| `BUILDDIR` | Auto | Custom build output directory |

## Installation

### Windows

Use one of the component installer scripts:

```bash
cd Install

# Agent installer
AgentinoAgent/install_ReleaseVC17_64.bat

# Server installer
AgentinoServer/install_ReleaseVC17_64.bat

# Client installer
AgentinoClient/install_ReleaseVC16_64.bat
```

### Linux/macOS

After building with CMake:

```bash
cd build
sudo cmake --install . --prefix /usr/local
```

Or manually copy the executables:

```bash
# Copy server
cp AgentinoServer /usr/local/bin/

# Copy agent
cp AgentinoAgent /usr/local/bin/

# Copy libraries
cp lib*.so* /usr/local/lib/
```

## Getting Started

### Starting the Server

```bash
# Run the Agentino Server
AgentinoServer

# With custom configuration
AgentinoServer --config /path/to/config.json
```

The server will start and listen for agent connections. The GUI will display the topology view showing connected agents.

### Deploying an Agent

```bash
# On a managed node, run the agent
AgentinoAgent --server <server-address> --port <port>

# Example
AgentinoAgent --server 192.168.1.100 --port 8080
```

The agent will connect to the server and register itself, then start monitoring local services.

### Using the Client

```bash
# Run the client application
AgentinoClient --server <server-address> --port <port>
```

The client provides a GUI for:
- Viewing all registered agents
- Managing services (start, stop, restart)
- Monitoring service status in real-time
- Viewing service logs
- Configuring service parameters

### GraphQL API

Agentino exposes a GraphQL endpoint for programmatic access:

```graphql
# Query all agents
query {
  agents {
    id
    name
    status
    services {
      id
      name
      status
    }
  }
}

# Subscribe to service status changes
subscription {
  serviceStatusChanged {
    serviceId
    status
    timestamp
  }
}

# Start a service
mutation {
  startService(serviceId: "service-123") {
    success
    message
  }
}
```

## Development

### Generating Documentation

Documentation is generated using Doxygen:

```bash
cd Docs
doxygen Doxyfile

# Or on Windows
GenerateDocs.bat
```

The generated documentation will be in `Docs/html/`.

### Code Organization

- **Headers** (`Include/`) - Public interfaces and API definitions
- **Implementations** (`Impl/`) - Concrete implementations and applications
- **SDL** (`Sdl/`) - Service Definition Language files for data models
- **Resources** - UI resources, translations, and assets

### Component Architecture

Agentino uses a component-based architecture with:
- **Interfaces** (I prefix) - Abstract interfaces defining contracts
- **Components** (C prefix, Comp suffix) - Concrete implementations
- **Controllers** (Controller suffix) - Business logic and coordination
- **Wrappers** (Wrap suffix) - Adapters for external systems

## Security and Compliance

### Security

Agentino takes security seriously. For information about:

- **Reporting Security Vulnerabilities**: See [SECURITY.md](SECURITY.md)
- **Security Updates**: Review our [security policy](SECURITY.md)
- **Security Best Practices**: Consult the security guidelines in [SECURITY.md](SECURITY.md)

If you discover a security vulnerability, please report it responsibly through GitHub's private vulnerability reporting feature or by contacting the project maintainers.

### EU Cyber Resilience Act (CRA) Compliance

Agentino is designed to support EU Cyber Resilience Act compliance:

- **CRA Compliance Documentation**: [CRA_COMPLIANCE.md](CRA_COMPLIANCE.md)
- **Software Bill of Materials (SBOM)**: [SBOM.md](SBOM.md), [sbom.json](sbom.json), and [sbom.spdx.json](sbom.spdx.json)
- **Security Updates**: Regular security patches for supported versions
- **Vulnerability Management**: Documented process for handling security issues
- **SPDX License Identifiers**: All source files include SPDX license identifiers

For complete information about CRA compliance, see [CRA_COMPLIANCE.md](CRA_COMPLIANCE.md).

## License

Agentino is available under a commercial license only:

- **Commercial License** - For proprietary and commercial use

See the `Install/Commercial/` directory for license text.

## Version

Current version: **2023** (Service Manager Version ID)

## Support and Contact

For support, issues, or contributions, please refer to the project's issue tracker or contact the development team.

## Acknowledgments

- Built on the **ImtCore Framework** - Enterprise application framework
- Uses **Qt Framework** - Cross-platform application development
- Implements **GraphQL** - Modern API query language
- Leverages **ACF** - Application Component Framework

---

**Note**: This project requires the proprietary ImtCore framework which must be obtained and installed separately.
