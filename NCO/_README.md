# Layer 4.5 Revoke Unit Tests

Acronyms:
1) NCO: Network-wide Customization Orchestrator
2) DCA: Device Customization Agent
3) CIB: Customization Information Base


## Overview:
The Layer 4.5 customization framework is aimed at enhancing network operations through dynamic customization modules. It includes a set of scripts such as `nco_revoke.py`, `NCO_construct.py`, `NCO_deploy.py`, `NCO_logging.py`, `NCO_middlebox.py`, `NCO_monitor.py`, `NCO_security.py`, and `NCO.py`, each serving specific functions within this framework.
`nco_revoke.py` is essential for managing the lifecycle of customization modules across the network. It provides mechanisms for the Network-wide Customization Orchestrator (NCO) to revoke or deprecate modules on network devices, ensuring the network remains updated and secure.`NCO_construct.py` prepares the deployment environment for customization modules, handling prerequisites such as security configurations and module dependencies. This step is critical for the successful deployment and operation of modules. Following preparation, `NCO_deploy.py` oversees the module deployment process, ensuring the correct installation and activation of modules throughout the network. This script is key to implementing the network's customization strategy effectively.`NCO_logging.py` supports the customization process by offering logging capabilities for monitoring and debugging. It generates detailed logs of the framework's operations, which are vital for troubleshooting.`NCO_middlebox.py` manages the dependencies and interactions between customization modules, addressing module requirements and relationships to maintain the integrity of network customizations.`NCO_monitor.py` monitors the health and operation of deployed modules, tracking their performance and functionality to ensure network optimization and to quickly resolve any issues. Security is addressed by `NCO_security.py`, which protects the customization process and deployed modules from unauthorized access and threats. It implements encryption and key management among other security measures. Lastly, `NCO.py` acts as the orchestrator, combining the functionalities provided by the other scripts. It handles tasks such as device communication, command processing, and lifecycle management of customization modules, enabling the operation of the Network-wide Customization Orchestrator (NCO).


## Functional Components:
- Revoke: Supports the removal of outdated or misbehaving customization modules from devices.
- Deprecate: Allows for the phased transition of customization modules, enabling a smooth update process to newer versions without disrupting network operations.
- Construct: Prepares the environment for deploying customization modules, ensuring all prerequisites are met.
- Deploy: Manages the deployment of modules across the network, ensuring modules are correctly installed and activated.
- Monitor: Oversees the operation of deployed modules, ensuring their performance and functionality meet network requirements.
- Secure: Implements security measures to protect the customization process and deployed modules from unauthorized access and threats.
- Log: Provides comprehensive logging capabilities to monitor and debug the customization framework's operations.


Scripts and Test Cases: 

1. NCO.py:
    - Acts as the central orchestrator for the Layer 4.5 network customization framework, integrating various functionalities.
    - Key Features:
        - Main entry point for executing operations, handling communication, processing commands, and managing the lifecycle of customization modules.

2. NCO_construct.py:
    - Supports the construction and preparation environment for deploying customization modules, including security aspects like key generation.
    - Key Functions:
        - request_symver_file: Handles the symver file necessary for building modules remotely.

3. NCO_deploy.py:
    - Manages the deployment lifecycle of customization modules, ensuring the database reflects the current deployment status.
    - Key Functions:
        - handle_deployed_update: Manages updates based on deployment status and compares reported deployment lists with database entries.

4. NCO_logging.py:
    - Configures logging for monitoring and debugging across the network customization processes.
    - Key Features:
        - logger_configurer: Sets up the logging environment with rotating file handlers and console logging.

5. NCO_middlebox.py:
    - Manages dependencies and compatibility of customization modules across network devices.
    - Key Functions:
        - update_inverse_module_requirements: Handles module requirements and relationships.

6. NCO_monitor.py:
    - Oversees the status and health of customization modules across network devices.
    - Key Functions:
        - request_report: Collects operational data from devices to ensure network customization integrity.

7. NCO_revoke.py:
    - Manages the revocation and deprecation of customization modules on devices.
    - Interacts with the CIB for module state management and communicates with devices
      to apply these changes.
    - Key Functions:
        - handle_revoke_update: Updates the database for modules marked for revocation.
        - retrieve_revoke_list: Fetches a list of modules slated for revocation.
        - revoke_module: Sends revocation commands to devices.
        - retrieve_deprecated_list: Retrieves a list of modules marked for deprecation.
        - deprecate_module: Sends deprecation commands to devices.

8. NCO_security.py:
    - Ensures the confidentiality and integrity of data within the customization process through encryption and key management.
    - Key Functions:
        - generate_key: Focuses on secure key generation for communications and operations.
 

## Next Steps:
- Integrate these functionalities within the broader Layer 4.5 customization framework,
  ensuring seamless module management as part of the network-wide customization efforts.
- Refer to the Layer 4.5 documentation and sample modules for further experimentation and
  to understand the application of these mechanisms in real-world scenarios.


