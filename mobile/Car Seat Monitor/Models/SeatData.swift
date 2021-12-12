//
//  SeatData.swift
//  Car Seat Monitor
//
//  Created by Emina on 27. 10. 2021..
//

import Foundation

struct SeatData {
  let communicating_0: Bool
  let communicating_1: Bool
  let present: Bool
  let belt: Bool
  let battery_0: Bool
  let battery_1: Bool
  let temperature: Float
  let batteryMain: Bool
  let driverPresent: Bool
  let secondsUntilDriverMissingAlarm: Int!
}
