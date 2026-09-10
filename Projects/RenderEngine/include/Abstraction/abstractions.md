# Abstractions

Abstraction classes that encapsulate the behavior of different `d3d11` COM-objects, 
providing easy interfaces for creation, pipeline binding and resource updating.

## DX-prefix

The DX (short for DirectX) was picked to separate abstraction types from the ordinary ID3D11-interface objects. The DX-prefix is only applied to the base-level wrappers that contain one or multiple `ComPtr<ID3D11DeviceChild>` member variables.