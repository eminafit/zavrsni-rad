//
//  EnterPhoneView.swift
//  Car Seat Monitor
//
//  Created by Emina on 2. 11. 2021..
//

import SwiftUI

struct EnterPhoneView: View {
  @Binding var phone: String
  @State var ownPhone = ""

  var body: some View {
    Form {
      TextField("New Phone Number", text: $ownPhone)
        .keyboardType(.phonePad)
    }
    .navigationBarTitle("Add Phone Number")
    .onAppear(perform: {
      self.ownPhone = self.phone
    })
    .onDisappear(perform: {
      self.phone = self.ownPhone
    })
  }
}
