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
- Deprecate: Allows for the phased transition of customization modules, enabling a smooth
  update process to newer versions without disrupting network operations.

Scripts and Test Cases:

1. NCO_revoke.py:
    - Manages the revocation and deprecation of customization modules on devices.
    - Interacts with the CIB for module state management and communicates with devices
      to apply these changes.
    - Key Functions:
        - handle_revoke_update: Updates the database for modules marked for revocation.
        - retrieve_revoke_list: Fetches a list of modules slated for revocation.
        - revoke_module: Sends revocation commands to devices.
        - retrieve_deprecated_list: Retrieves a list of modules marked for deprecation.
        - deprecate_module: Sends deprecation commands to devices.

2. test_nco_revoke.py:
    - Contains unit tests to verify the functionality of module revocation and deprecation.
    - Tests ensure the integrity and correctness of the revocation/deprecation processes,
      aligning with the framework's requirements for dynamic module management.
    - Key Tests:
        - test_handle_revoke_update: Validates database updates for revoked modules.
        - test_retrieve_revoke_list_direct_return: Ensures accurate retrieval of revocation lists.
        - test_revoke_module: Checks the revocation command functionality and database updates.
        - test_retrieve_deprecated_list: Validates the retrieval of deprecation lists.
        - test_deprecate_module: Tests the deprecation command functionality.

## Next Steps:
- Integrate these functionalities within the broader Layer 4.5 customization framework,
  ensuring seamless module management as part of the network-wide customization efforts.
- Refer to the Layer 4.5 documentation and sample modules for further experimentation and
  to understand the application of these mechanisms in real-world scenarios.


