//
//  OBSCameraProviderSource.swift
//  camera-extension
//
//  Created by Sebastian Beckmann on 2022-09-30.
//  Changed by Patrick Heyer on 2022-10-16.
//

import CoreMediaIO
import Foundation

class OBSCameraProviderSource: NSObject, CMIOExtensionProviderSource {
    private(set) var provider: CMIOExtensionProvider!

    private var deviceSource: OBSCameraDeviceSource!

    init(clientQueue: DispatchQueue?, deviceUUID: UUID, sourceUUID: UUID, sinkUUID: UUID) {
        super.init()

        provider = CMIOExtensionProvider(source: self, clientQueue: clientQueue)
        deviceSource = OBSCameraDeviceSource(
            localizedName: "PRISM Live Studio", //PRISM/Zhongling/20231123/#/VCAM
            deviceUUID: deviceUUID,
            sourceUUID: sourceUUID,
            sinkUUID: sinkUUID)
        do {
            try provider.addDevice(deviceSource.device)
        } catch let error {
            fatalError("Failed to add device \(error.localizedDescription)")
        }
    }

    func connect(to client: CMIOExtensionClient) throws {
    }

    func disconnect(from client: CMIOExtensionClient) {
    }

    var availableProperties: Set<CMIOExtensionProperty> {
        return [.providerName, .providerManufacturer]
    }

    func providerProperties(forProperties properties: Set<CMIOExtensionProperty>) throws
        -> CMIOExtensionProviderProperties
    {
        let providerProperties = CMIOExtensionProviderProperties(dictionary: [:])

        if properties.contains(.providerName) {
            providerProperties.name = "PRISM Camera Extension Provider" //PRISM/Zhongling/20231123/#/VCAM
        }

        if properties.contains(.providerManufacturer) {
            providerProperties.manufacturer = "PRISM Project" //PRISM/Zhongling/20231123/#/VCAM
        }

        return providerProperties
    }

    func setProviderProperties(_ providerProperties: CMIOExtensionProviderProperties) throws {
    }
}
