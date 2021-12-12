//
//  BLEManager.swift
//  Car Seat Monitor
//
//  Created by Emina on 27. 10. 2021..
//

import Foundation
import CoreBluetooth

class BLEManager: NSObject, ObservableObject, CBPeripheralDelegate, CBCentralManagerDelegate {
  @Published var connected = false
  @Published var seatInfo:SeatData!
  @Published var authorization = true

  private var myCentral: CBCentralManager!
  private var peripheral: CBPeripheral!
  private var txCharacteristic: CBCharacteristic!
  private var rxCharacteristic: CBCharacteristic!
  private var receivedData:String = ""

  
  override init() {
    super.init()
    
    self.myCentral = CBCentralManager(delegate: self, queue: nil)
    self.myCentral.delegate = self
  }
  
  func centralManagerDidUpdateState(_ central: CBCentralManager) {
    if(central.state == .poweredOn) {
      self.myCentral.scanForPeripherals(withServices: [CBUUIDs.BLE_Service_UUID],
                                        options: nil)
    } else if(central.state == .unauthorized) {
      self.authorization = false
    }
  }
  
  
  func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
    let identifier = peripheral.identifier
    print ("Advertisement Data : \(advertisementData)")
    // Is this specific test device that we are looking for?
    // In final version we should offer user to select device he wants to connect,
    // but in this prototype phase we are looking for specific device.
    if (identifier == CBUUIDs.BLE_Device_UUID) {
      print("Found Car Seat Monitor")
      self.myCentral.stopScan()
      self.peripheral = peripheral
      self.peripheral.delegate = self
      self.myCentral.connect(self.peripheral, options: nil)
    }
  }
  
  public func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
    if peripheral == self.peripheral {
      print("Connected to Car Seat Monitor")
      self.peripheral.discoverServices([CBUUIDs.BLE_Service_UUID])
    }
  }
  
  func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
    self.connected = false
    self.peripheral = nil
    self.myCentral.scanForPeripherals(withServices: [CBUUIDs.BLE_Service_UUID], options: nil)
  }
  
  func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
    if ((error) != nil) {
      print("Error discovering services: \(error!.localizedDescription)")
      return
    }
    guard let services = peripheral.services else {
      return
    }
    for service in services {
      peripheral.discoverCharacteristics(nil, for: service)
    }
    print("Discovered Services: \(services)")
  }
  
  func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
    
    guard let characteristics = service.characteristics else {
      return
    }
    
    print("Found \(characteristics.count) characteristics.")
    print("Characteristics---:")
    for characteristic in characteristics {
      
      if characteristic.uuid.isEqual(CBUUIDs.BLE_Characteristic_uuid_Rx)  {
        rxCharacteristic = characteristic
        peripheral.setNotifyValue(true, for: rxCharacteristic!)
        peripheral.readValue(for: characteristic)
        print("RX Characteristic: \(rxCharacteristic.uuid)")
      }
      
      if characteristic.uuid.isEqual(CBUUIDs.BLE_Characteristic_uuid_Tx){
        txCharacteristic = characteristic
        print("TX Characteristic: \(txCharacteristic.uuid)")
        print("Characteristic properties: \(characteristic.properties)")
      }
      print(characteristic)
    }
    print("---END")
    self.connected = true
  }
  
  func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
    var characteristicASCIIValue = NSString()
    print(characteristic)
    guard characteristic == rxCharacteristic,
          let characteristicValue = characteristic.value,
          let ASCIIstring = NSString(data: characteristicValue, encoding: String.Encoding.utf8.rawValue) else { return }
    characteristicASCIIValue = ASCIIstring
    print("Value Recieved: \((characteristicASCIIValue as String))")
    self.receivedData = self.receivedData + String(ASCIIstring)
    if(ASCIIstring.substring(from: ASCIIstring.length - 1) == "\n") {
      self.receivedData = self.receivedData.trimmingCharacters(in: CharacterSet.newlines) // Remove new line char at the end
      let seatDataArray = self.receivedData.components(separatedBy: ";")
      if(seatDataArray.count == 10) {
        print(seatDataArray as Any)
        self.seatInfo = SeatData(communicating_0: NSString(string: seatDataArray[0]).boolValue,
                                 communicating_1: NSString(string: seatDataArray[3]).boolValue,
                                 present: NSString(string: seatDataArray[2]).boolValue,
                                 belt: NSString(string: seatDataArray[6]).boolValue,
                                 battery_0: NSString(string: seatDataArray[1]).boolValue,
                                 battery_1: NSString(string: seatDataArray[4]).boolValue,
                                 temperature: Float(seatDataArray[5]) ?? 0.0,
                                 batteryMain: NSString(string: seatDataArray[7]).boolValue,
                                 driverPresent: NSString(string: seatDataArray[8]).boolValue,
                                 secondsUntilDriverMissingAlarm: Int(seatDataArray[9]) ?? nil)
      }
      self.receivedData = ""
      print(self.seatInfo as Any)
    }
  }
  
  func writeToDevice(contacts: [String]) {
    if let peripheral = peripheral {
      if let txCharacteristic = txCharacteristic {
        print("Here")
        print(contacts)
        for (index, contact) in contacts.enumerated() {
          let removeCharacters: Set<Character> = [" ", "(", ")"]
          var phone = contact
          phone.removeAll(where: { removeCharacters.contains($0) } )
          let value = "\(index):\(phone)\n"
          let data = (value as NSString).data(using: String.Encoding.utf8.rawValue)
          peripheral.writeValue(data!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
        }
        for n in (contacts.count)...5 {
          if n == 5 { break }
          let data = ("\(n):\n" as NSString).data(using: String.Encoding.utf8.rawValue)
          peripheral.writeValue(data!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
        }
      }
    }
  }
  
  func extendDelay() {
    if let peripheral = peripheral {
      let data = ("D\n" as NSString).data(using: String.Encoding.utf8.rawValue)
      peripheral.writeValue(data!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
    }
  }
  
  func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
    guard error == nil else {
        print("Error writing data: ")
        print(error as Any)
        return
      }
      print("Message sent")
  }
}
