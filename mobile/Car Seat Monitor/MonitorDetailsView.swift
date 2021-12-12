//
//  MonitorDetailsView.swift
//  Car Seat Monitor
//
//  Created by Sasa on 16. 11. 2021..
//

import SwiftUI

struct MonitorDetailsView: View {
  var seatInfo:SeatData
  var highTemperatureLimit:Float
  
  var body: some View {
    if(seatInfo.batteryMain) {
      Text("Main device battery is charged").foregroundColor(.green)
    } else {
      Text("Main device battery needs charging").foregroundColor(.red)
    }
    if(seatInfo.communicating_0) {
      if seatInfo.battery_0 {
        Text("Presence monitor battery is charged").foregroundColor(.green)
      } else {
        Text("Presence monitor battery needs charging").foregroundColor(.red)
      }
    } else {
      Text("Presence monitor not communicating").foregroundColor(.red)
    }
    if seatInfo.communicating_1 {
      if seatInfo.battery_1 {
        Text("Belt monitor battery is charged").foregroundColor(.green)
      } else {
        Text("Belt monitor battery needs charging").foregroundColor(.red)
      }
    } else {
      Text("Belt monitor not communicating").foregroundColor(.red)
    }
    if seatInfo.communicating_0 && seatInfo.communicating_1 {
      if seatInfo.present {
        Text("Child is present in the seat").foregroundColor(.green)
      } else {
        Text("Child is not present in the seat").foregroundColor(.red)
      }
    }
    if seatInfo.communicating_1 {
      if seatInfo.communicating_0 && seatInfo.present {
        if seatInfo.belt {
          Text("Child seat belt is ON").foregroundColor(.green)
        } else {
          Text("Child seat belt is OFF").foregroundColor(.red)
        }
        Text(String(format: "Temperature: %.1f°C", seatInfo.temperature))
          .foregroundColor((seatInfo.temperature < highTemperatureLimit) ? .green : .red)

      }
    }
  }
}

