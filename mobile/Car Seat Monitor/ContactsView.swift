//
//  ContactsView.swift
//  Car Seat Monitor
//
//  Created by Emina on 27. 10. 2021..
//

import SwiftUI
import ContactsUI

struct ContactsView: View {
  @Binding var contacts :[String]
  @State private var ownContacts :[String] = []
  @State var newPhone = ""
  @Environment(\.editMode) var editMode
  
  var body: some View {
    VStack(spacing: 20) {
      List {
        Text("Please make sure that first phone number in the list is your own. That way you will be first to receive any SMS alarm.")
          .font(.footnote)
        ForEach(ownContacts, id: \.self) { contact in
          Text(contact)
        }
        .onDelete(perform: deleteContact)
        .onMove(perform: moveContact)
        if(!(editMode?.wrappedValue.isEditing ?? false) && ownContacts.count < 5) {
          HStack{
            Image(systemName: "phone.fill.badge.plus").foregroundColor(.green)
            NavigationLink(destination: EnterPhoneView(phone: $newPhone),
                           label: { Text("Enter Phone Number") })
          }
        }
      }
      Spacer()
    }
    .animation(nil)
    .navigationTitle("Emergency Contacts")
    .navigationBarTitleDisplayMode(.inline)
    .toolbar {
      EditButton()
    }
    .onAppear(perform: setOwnContacts)
    .onDisappear(perform: setContacts)
    .onChange(of: newPhone) { phone in
      if(phone != "") {
        self.ownContacts.append(phone)
        self.newPhone = ""
      }
    }
  }
  
  func setOwnContacts() {
    self.ownContacts = self.contacts
  }
  
  func setContacts() {
    self.contacts = self.ownContacts
  }
  
  func deleteContact(at offsets: IndexSet) {
    ownContacts.remove(atOffsets: offsets)
  }
  
  func moveContact(from source: IndexSet, to destination: Int) {
    ownContacts.move(fromOffsets: source, toOffset: destination)
  }
}

