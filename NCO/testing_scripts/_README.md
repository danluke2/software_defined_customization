# Layer 4.5 Revoke Unit Tests

Acronyms:
1) NCO: Network-wide Customization Orchestrator
2) DCA: Device Customization Agent
3) CIB: Customization Information Base

## Overview:
The script `nco_revoke.py` plays a crucial role in the Layer 4.5 customization framework by providing
the necessary functionalities to manage the lifecycle of customization modules deployed across the network.
Its primary purpose is to enable the Network-wide Customization Orchestrator (NCO) to dynamically revoke
(outdate and remove) or deprecate (phase out in preparation for removal or replacement) customization modules
on network devices. This capability ensures that the network remains adaptable, secure, and aligned with
current customization needs, facilitating a robust and flexible network customization layer.

The unit test file `test_nco_revoke.py` is designed to validate the correct implementation of module revocation
and deprecation functionalities in `nco_revoke.py`. Through a series of automated tests, it aims to ensure
the reliability and stability of these critical processes within the Layer 4.5 framework. This includes verifying
the accurate interaction with the database for state management, the correct execution of commands for module
management on devices, and the overall integrity of the revocation and deprecation mechanisms.

## Functional Components:
- Revoke: Supports the removal of outdated or misbehaving customization modules from devices.
- Deprecate: Allows for the phased transition of customization modules, enabling a smooth update process to newer versions without disrupting network operations.
- Construct: Prepares the environment for deploying customization modules, ensuring all prerequisites are met.
- Deploy: Manages the deployment of modules across the network, ensuring modules are correctly installed and activated.
- Monitor: Oversees the operation of deployed modules, ensuring their performance and functionality meet network requirements.
- Secure: Implements security measures to protect the customization process and deployed modules from unauthorized access and threats.
- Log: Provides comprehensive logging capabilities to monitor and debug the customization framework's operations.


Scripts and Test Cases:

1. test_nco_revoke.py:
    - Contains unit tests to verify the functionality of module revocation and deprecation.
    - Tests ensure the integrity and correctness of the revocation/deprecation processes,
      aligning with the framework's requirements for dynamic module management.
    - Key Tests:
        - test_handle_revoke_update: Validates database updates for revoked modules.
        - test_retrieve_revoke_list_direct_return: Ensures accurate retrieval of revocation lists.
        - test_revoke_module: Checks the revocation command functionality and database updates.
        - test_retrieve_deprecated_list: Validates the retrieval of deprecation lists.
        - test_deprecate_module: Tests the deprecation command functionality.




