//
//  ContentView.swift
//  Car Seat Monitor
//
//  Created by Emina on 26. 10. 2021..
//

import SwiftUI
import UserNotifications

struct ContentView: View {
  @StateObject var bleManager = BLEManager()
  @State var authorizedNotifications = true
  @State var contacts :[String] = []
  @State var communicating = false
  @State var comm0Alarm = false
  @State var comm1Alarm = false
  @State var battery0Alarm = false
  @State var battery1Alarm = false
  @State var batteryMainAlarm = false
  @State var highTemperatureAlarm = false
  @State var driverNotPresentAlarm = false
  @State var driverNotPresentRepeatedAlarm = false
  @State var alarmTimeExtended = false
  @State var numberOfextends = 0
  let highTemperatureLimit:Float = 32.0
  
  var body: some View {
    NavigationView {
      List {
        if (!bleManager.authorization) {
          Text("You did not grant us authorization to use Bluetooth. That defets purpose of this application. Please allow this app to use Bluetooth in your phone settings. Go to Settings > Privacy > Bluetooth and toggle Car Seat Monitor button.").foregroundColor(.red)
        } else if (!bleManager.connected) {
          Text("Scanning for Car Seat Monitor device...").foregroundColor(.red)
        } else if (bleManager.seatInfo != nil) {
          MonitorDetailsView(seatInfo: bleManager.seatInfo,
                             highTemperatureLimit: self.highTemperatureLimit)
          if !bleManager.seatInfo.driverPresent && alarmingMakesSense(data: bleManager.seatInfo) {
            VStack {
              let minutes:Int = bleManager.seatInfo.secondsUntilDriverMissingAlarm != nil ? bleManager.seatInfo.secondsUntilDriverMissingAlarm / 60 : -1
              HStack {
                Text("You are not in the Car").foregroundColor(.orange)
                Spacer()
                if(bleManager.seatInfo.secondsUntilDriverMissingAlarm == nil) {
                  Text("Please take a seat").foregroundColor(.orange)
                } else if(bleManager.seatInfo.secondsUntilDriverMissingAlarm == 0) {
                  Text("Alarming").foregroundColor(.red)
                } else if(minutes == 0) {
                  Text("<1m to alarm").foregroundColor(.red)
                } else {
                  if(minutes == 5) {
                    Text("5m before alarm").foregroundColor(.green)
                  } else {
                    Text("<\(minutes + 1)m before alarm").foregroundColor(.orange)
                  }
                }
              }
              // Can extend time only when:
              // No temperature alarms
              // Did not extend more than twice before
              // We are in last minute before SMS alarming
              if(bleManager.seatInfo.secondsUntilDriverMissingAlarm != 0 && minutes == 0 && bleManager.seatInfo.temperature < self.highTemperatureLimit && self.numberOfextends < 3) {
                if(!self.alarmTimeExtended) {
                  Button(action: {
                    extendTimeBeforeAlarm()
                  }){ Text("Extend time before alarm to 5 minutes").padding() }
                } else {
                  Spacer()
                  Text("Sending delay instructions to the device...")
                }
              }
            }
          }
        } else {
          Text("Connected to Car Seat Monitor\nWaiting for data...").foregroundColor(.green)
        }
        if(!self.authorizedNotifications) {
          Text("You did not grant us authorization to send you any notification. That defets purpose of this application. Please enable notifications in your phone settings. Go to Settings > Notifications > Car Seat Monitor and toggle 'Allow Notifications'").foregroundColor(.red)
        }
      }
      .navigationTitle("Seat Monitor Info")
      .navigationBarTitleDisplayMode(.inline)
      .navigationViewStyle(StackNavigationViewStyle())
      .toolbar {
        NavigationLink(destination: ContactsView(contacts: $contacts)) {
          Text("Contacts")
        }
      }
    }
    .onAppear(perform: startUp)
    .onChange(of: contacts) { _ in
      saveContacts()
    }
    .onReceive(bleManager.$connected) { newValue in
      if(!communicating && newValue) {
        communicating = true
        sendContacts()
      } else if (!newValue && communicating && alarmingMakesSense(data: bleManager.seatInfo)) {
        print("Lost connection")
        sendNotification(type: NotificationType.connLost)
      }
    }
    .onReceive(bleManager.$seatInfo) { newValue in
      checkSeatInfo(data: newValue)
    }
  }
  
  func alarmingMakesSense(data: SeatData) -> Bool {
    return (data.communicating_0 && data.communicating_1 && data.present)
  }
  
  func checkSeatInfo(data: SeatData!) {
    // Child not in seat or missing data, just return
    if(data == nil || !data.present) { return }
    // Alarming is not needed
    if(!alarmingMakesSense(data: data)) { return }
    
    // Check communication with child presence monotoring device
    if(!data.communicating_0 && !self.comm0Alarm) {
      self.comm0Alarm = true
      sendNotification(type: .noComm0)
    }
    if(self.comm0Alarm && data.communicating_0) { self.comm0Alarm = false }
    // Check communication with child belt and temperature monotoring device
    if(!data.communicating_1 && !self.comm1Alarm) {
      self.comm1Alarm = true
      sendNotification(type: .noComm1)
    }
    if(self.comm1Alarm && data.communicating_1) { self.comm1Alarm = false }
    // Check battery of child presence monotoring device
    if(!data.battery_0 && !self.battery0Alarm && data.communicating_0) {
      self.battery0Alarm = true
      sendNotification(type: .battery0)
    }
    if(self.battery0Alarm && data.battery_0 && data.communicating_0) { self.battery0Alarm = false }
    // Check battery of child belt and temperature monotoring device
    if(!data.battery_1 && !self.battery1Alarm && data.communicating_1) {
      self.battery1Alarm = true
      sendNotification(type: .battery1)
    }
    if(self.battery1Alarm && data.battery_1 && data.communicating_1) { self.battery1Alarm = false }
    // Check battery of maing device
    if(!data.batteryMain && !self.batteryMainAlarm) {
      self.batteryMainAlarm = true
      sendNotification(type: .batteryMain)
    }
    if(self.batteryMainAlarm && data.batteryMain) { self.batteryMainAlarm = false }
    // Check optimal temperature
    if(data.temperature >= highTemperatureLimit && !self.highTemperatureAlarm) {
      self.highTemperatureAlarm = true
      sendNotification(type: .highTemperature)
    }
    if(self.highTemperatureAlarm && (data.temperature < highTemperatureLimit)) { self.highTemperatureAlarm = false }
    // Reset time extending?
    if(data.secondsUntilDriverMissingAlarm != nil && data.secondsUntilDriverMissingAlarm > 60) {
      alarmTimeExtended = false;
    }
    // Check driver present
    if(!data.driverPresent && !self.driverNotPresentAlarm) {
      self.driverNotPresentAlarm = true
      driverNotPresentRepeatedAlarm = true
      sendNotification(type: .driverAway)
    }
    if(!data.driverPresent && data.secondsUntilDriverMissingAlarm != nil &&
        data.secondsUntilDriverMissingAlarm <= 60 && !self.driverNotPresentRepeatedAlarm) {
      driverNotPresentRepeatedAlarm = true
      sendNotification(type: .driverAway)
    }
    if(self.driverNotPresentAlarm && data.driverPresent) {
      self.driverNotPresentAlarm = false
      self.driverNotPresentRepeatedAlarm = false
      self.alarmTimeExtended = false
      self.numberOfextends = 0
    }
  }
  
  func startUp() {
    readContacts()
    UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .badge, .sound])  { success, error in
      if success {
        print("Authorization granted")
        self.authorizedNotifications = true
      } else {
        print(error as Any)
        self.authorizedNotifications = false
      }
    }
  }
  
  func readContacts() {
    if let data = UserDefaults.standard.data(forKey: "contacts") {
      if let decoded = try? JSONDecoder().decode([String].self, from: data) {
        self.contacts = decoded
        return
      }
    }
    self.contacts = []
  }
  
  func saveContacts() {
    if let encoded = try? JSONEncoder().encode(self.contacts) {
      print("Saving!!!")
      UserDefaults.standard.set(encoded, forKey: "contacts")
      sendContacts()
    }
  }
  
  func sendContacts() {
    print("Sending contacts to Device")
    if(self.communicating) {
      print("Sending...")
      bleManager.writeToDevice(contacts: self.contacts)
    } else {
      print("Error: Not connected")
    }
  }
  
  func extendTimeBeforeAlarm() {
    if(self.communicating) {
      print("extending time...")
      bleManager.extendDelay()
      alarmTimeExtended = true
      driverNotPresentRepeatedAlarm = false
      numberOfextends += 1
    } else {
      print("Error: Not connected")
    }
  }
  
  func sendNotification(type:NotificationType) {
    let content = UNMutableNotificationContent()
    switch type {
    case .connLost:
      content.title = "Connection Lost"
      content.body = "Connection with Car Seat Monitor lost!"
    case .highTemperature:
      content.title = "Temperature is too high"
      content.body = "Temperature in the car is too high. Please take the child out of the car."
    case .driverAway:
      content.title = "You are moving away from the car"
      content.body = "Please do not leave child in the car alone. After one minute we will start contacting your family and friends. You can extend that time by 5 minutes in user interface of the app."
    case .battery0:
      content.title = "Battery needs recharging"
      content.body = "Battery on child presense checking device needs recharging."
    case .battery1:
      content.title = "Battery needs recharging"
      content.body = "Battery on child belt and temperature checking device needs recharging."
    case .batteryMain:
      content.title = "Battery needs recharging"
      content.body = "Battery on main device needs recharging. Pleaase connect device to your car lighter port."
    case .noComm0:
      content.title = "Communication lost"
      content.body = "Communication with child presense checking device lost!"
    case .noComm1:
      content.title = "Communication lost"
      content.body = "Communication with child belt and temperature checking device lost!"
    }
    content.sound = UNNotificationSound.default
    let trigger = UNTimeIntervalNotificationTrigger(timeInterval: 1, repeats: false)
    let request = UNNotificationRequest(identifier: UUID().uuidString,
                                        content: content,
                                        trigger: trigger)
    UNUserNotificationCenter.current().add(request) { error in
      if let error = error {
        print(error)
      }
    }
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
