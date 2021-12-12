//
//  CBUUIDs.swift
//  Car Seat Monitor
//
//  Created by Emina on 19. 11. 2021..
//

import Foundation
import CoreBluetooth

struct CBUUIDs {
  static let dBLE_UUID = "99C0A6AA-2904-5D8A-A2C3-24788152527F" // Specific device UUID
  static let kBLE_Service_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e" // Service CBUUID
  static let kBLE_Characteristic_uuid_Tx = "6e400002-b5a3-f393-e0a9-e50e24dcca9e" // Transmit charateristic CBUUID
  static let kBLE_Characteristic_uuid_Rx = "6e400003-b5a3-f393-e0a9-e50e24dcca9e" // Receive charateristic CBUUID
  static let BLE_Device_UUID = UUID(uuidString: dBLE_UUID)
  static let BLE_Service_UUID = CBUUID(string: kBLE_Service_UUID)
  static let BLE_Characteristic_uuid_Tx = CBUUID(string: kBLE_Characteristic_uuid_Tx)
  static let BLE_Characteristic_uuid_Rx = CBUUID(string: kBLE_Characteristic_uuid_Rx)
}
