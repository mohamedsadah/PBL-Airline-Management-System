# Airline Management System (PBL)

## Overview

The Airline Management System is a browser-based application designed and developed as a Project-Based Learning (PBL) initiative by the SkyCoders team at Graphic Era University, Dehradun. This system demonstrates how modern technologies can be leveraged to build efficient, persistent, and accessible airline operations software entirely on the client-side.

**Key Features:**
- Flight scheduling and management
- Passenger booking and seat assignment
- User account management with role-based access (Customer, Administrator)
- Operational reporting and notifications
- Persistent, client-side data storage (IndexedDB)

## Technology Stack

- **C** (Core logic and data structures)
- **WebAssembly** (Compiled from C via Emscripten for execution in the browser)
- **IndexedDB** (Client-side persistent database)
- **HTML5, CSS3, JavaScript** (Frontend and UI)
- **Visual Studio Code, GCC, Git** (Development tools)

## Architecture

1. **Core Logic (C/WebAssembly):**  
   Implements critical operations like flight management (using BST), passenger records (linked lists), and seat allocation (2D arrays). Compiled to WebAssembly for browser execution.
2. **Frontend (HTML, CSS, JS):**  
   Provides user interfaces for customers and administrators, handling interactions and communication with WebAssembly modules.
3. **Persistence (IndexedDB):**  
   Ensures that all data (flights, bookings, users) is stored securely and persistently in the browser.

## Functionalities

### For Administrators

- Add, update, or cancel flights
- View all flights and passenger lists
- Generate boarding passes

### For Customers

- Search flights by origin, destination, and date
- Book flights and receive automatic seat assignment
- View, download, or cancel bookings
- Receive notifications regarding bookings

## Getting Started

> **Note:** The system is designed for modern browsers supporting WebAssembly and IndexedDB.

1. **Clone the Repository:**
   ```sh
   git clone https://github.com/mohamedsadah/PBL-Airline-Management-System.git
   ```

2. **Open the Application:**
   - No server is required.
   - Simply open the main HTML file (`index.html`) in your browser.

3. **Development:**
   - Core C modules can be modified and recompiled to WebAssembly using [Emscripten](https://emscripten.org/).
   - Frontend code is in JavaScript, HTML, and CSS directories.

## Screenshots - sample

![Screenshot from 2025-06-08 21-35-14](https://github.com/user-attachments/assets/cd6f74b9-e5c7-42b6-9d9e-21ad76b66b53)

![Screenshot from 2025-06-08 21-37-18](https://github.com/user-attachments/assets/4bf37686-fc40-4645-8d73-408c977d747e)

![Screenshot from 2025-06-08 22-26-49](https://github.com/user-attachments/assets/1040b4ba-a06a-41a4-ad1e-e2e9492d3a21)

![Screenshot from 2025-06-08 22-27-37](https://github.com/user-attachments/assets/ed53aac9-62eb-4b98-9985-4407244b0e2b)

![Screenshot from 2025-06-08 21-41-24](https://github.com/user-attachments/assets/cadef1a5-b182-4880-9723-f7b9c62ccce4)

![Screenshot from 2025-06-08 22-18-01](https://github.com/user-attachments/assets/eaf96cc2-fea8-4740-93ea-07001d01a7ce)


## Project Structure

```
/
├── c_modules/            # Core logic in C
├── js/                   # Frontend JavaScript modules
├── css/                  # Stylesheets
├── html/                 # HTML user interface files
├── assets/               # Images and static resources
├── README.md
└── ...
```

## Contributing

Contributions are welcome! Please fork the repository and submit pull requests for improvements or bug fixes.

## License

This project is for educational purposes and may be reused or modified with attribution.

## Credits

### Developed by **SkyCoders**
- Ashutosh Kumar
- Mohamed Sadah Bah
- Sonali Kumari
- Rahul Kumar

### Mentor
- Ms Rashmi Kanyal
  > Assistant Professor

> Graphic Era University, Dehradun, Uttarakhand, India.

## References

- [Emscripten Documentation](https://emscripten.org/docs/)
- [IndexedDB API](https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API)
- Project documentation and guidelines
