// Emscripten Module setup
let searchOneWay,
  searchRoundTrip,
  allFlights = [];

var Module = {
  onRuntimeInitialized: function () {
    FS.mkdir("/persistent");
    FS.mount(IDBFS, {}, "/persistent");
    FS.syncfs(true, function (err) {
      if (err) {
        console.error("Error syncing FS:", err);
        return;
      }

      console.log("Persistent filesystem loaded.");
      Module.ccall("loadFlightsFromFile", null, [], []);

      const getAllFlightsJSON = Module.cwrap("getAllFlightsJSON", "string", []);
      const jsonData = getAllFlightsJSON();

      allFlights = JSON.parse(jsonData);
      console.log(allFlights.map((f) => f.flightNumber));

      // Register C-wrapped functions globally
      searchOneWay = Module.cwrap("getOneWayFlightsJSON", "string", [
        "string",
        "string",
        "string",
      ]);
    });
  },
};

let departure, destination, departDate, returnDate;
// Attach form event listener when DOM is ready
document.addEventListener("DOMContentLoaded", () => {
  const form = document.getElementById("form");
  const resultsDiv = document.querySelector(".searched-flights");

  document.querySelectorAll('input[name="trip"]').forEach((radio) => {
    radio.addEventListener("change", function () {
      const tripType = document.querySelector(
        'input[name="trip"]:checked'
      )?.value;
      const returnDateInput = document.getElementById("returnDate");

      if (tripType === "round") {
        returnDateInput.disabled = false;
      } else {
        returnDateInput.disabled = true;
        returnDateInput.value = "";
      }
    });
  });

  form.addEventListener("submit", function (e) {
    e.preventDefault(); // Prevent page reload
    resultsDiv.innerHTML = "";

    departure = document.getElementById("departure").value.trim();
    destination = document.getElementById("destination").value.trim();
    departDate = document.getElementById("departDate").value;
    returnDate = document.getElementById("returnDate").value;

    const today = new Date().toISOString().split("T");

    document.querySelectorAll('input[type="date"]').forEach((input) => {
      input.min = today[0];
    });

    const tripType = document.querySelector(
      'input[name="trip"]:checked'
    )?.value;

    if (!departure || !destination || !departDate) {
      showHero("Please fill all required fields.", "rgb(250, 215, 61)");
      return;
    }

    if (tripType === "round" && !returnDate) {
      showHero(
        "Please select return date for round trip.",
        "rgb(250, 215, 61)"
      );
      return;
    }

    if (tripType === "one") {
      const json = searchOneWay(departure, destination, departDate);
      console.log(`one way: ${json}`);
      renderFlightResults(
        json,
        `Flights from ${departure} to ${destination}`,
        departure,
        destination
      );
    } else {
      const to = searchOneWay(departure, destination, departDate);
      const fro = searchOneWay(destination, departure, returnDate);
      console.log(`roundto: ${to}`);
      console.log(`roundfro: ${fro}`);
      try {
        renderFlightResults(
          to,
          `Flights from ${departure} to ${destination}`,
          departure,
          destination
        );
        renderFlightResults(
          fro,
          `Return Flights from ${destination} to ${departure}`,
          destination,
          departure
        );
      } catch (err) {
        console.error("Invalid JSON from C:", json);
        resultsDiv.innerHTML = "<p>Error parsing flight data.</p>";
      }
    }

    window.location.href = "#flights-div";
  });
});

document.getElementById("book-flight").addEventListener("click", function (e) {
  if (!departure || !destination || !departDate) {
    showHero("Please fill all required fields.", "rgb(250, 215, 61)");
    window.location.href = "#hero";
    return;
  }
});

// Render flight cards
function renderFlightResults(jsonStr, title, departure, destination) {
  const flights = JSON.parse(jsonStr);
  const section = document.createElement("div");
  section.innerHTML = `<h2 class="title">${title}</h2>`;

  if (flights.length === 0) {
    section.innerHTML += `<p class="no-flight-display">flights from ${departure} to ${destination} not available at the moment</p>`;
  } else {
    flights.forEach((flight) => {
      const card = document.createElement("div");
      card.className = "flight-card";
      card.innerHTML = `<div class="flight-image">
      <img src="/images/fl.jpg" alt="Flight">
  </div>
  <div class="flight-details">
      <div class="flight-row">
          <span class="flight-number">${flight.flightNumber}</span>
          <span>${flight.departure} → ${flight.destination}</span>
          <span class="travel-time">${flight.departDate} - ${
        flight.Dtime
      }</span>
          <span class="price">₹${flight.price}</span>
      </div>
      <div class="flight-dates">
          <small>Stops: ${
            flight.stops && flight.stops.length > 0
              ? flight.stops.map((stop) => `<span>${stop}</span>`).join(", ")
              : "None"
          }</small>
          <small>Arrives: ${flight.arrivalDate} → ${flight.Atime}</small>
          <small>Status: ${flight.status}</small>
      </div>
      </div>
      <div>
      <button class="book">Book Flight</button>
      </div>
`;
      section.appendChild(card);

      card.querySelector(".book").addEventListener("click", () => {
        // Save selected flight data to localStorage
        localStorage.setItem("selectedFlight", JSON.stringify(flight));
        // Redirect to checkout page
        window.location.href = "./bookflight.html";
      });
    });
  }

  const resultsDiv = document.querySelector(".searched-flights");
  resultsDiv.appendChild(section);
}

//flight status

const statusDiv = document.querySelector(".checkStatus");

document
  .getElementById("flight-status")
  .addEventListener("click", function (e) {
    document.querySelector(".confirm-container").classList.add("show");

    document.getElementById("check").addEventListener("click", function (e) {
      e.preventDefault();
      const flnum = document.getElementById("flNum").value.trim();

      if (!flnum) {
        showHero("Please enter a Flight Number.", "rgb(250, 215, 61)");
        return;
      }

      let found = false;

      for (let flight of allFlights) {
        if (flight.flightNumber === flnum) {
          found = true;
          showHero(
            `Flight ${flight.flightNumber} from ${flight.departDate} to ${flight.arrivalDate} is ${flight.status}`,
            "rgb(84, 250, 51)"
          );
        }
      }
      if (!found) {
        showHero("Flight does not exist.", "rgb(250, 215, 61)");
      }
    });

    document.getElementById("close").addEventListener("click", function (e) {
      document.querySelector(".confirm-container").classList.remove("show");
    });
  });

function showHero(Message, color) {
  const hero = document.createElement("div");
  hero.textContent = Message;
  hero.style.position = "fixed";
  hero.style.top = "20px";
  hero.style.left = "80px";
  hero.style.backgroundColor = color;
  hero.style.color = "rgb(15, 45, 59)";
  hero.style.padding = "25px 35px";
  hero.style.borderRadius = "10px";
  hero.style.boxShadow = "0 4px 12px rgba(0,0,0,0.15)";
  hero.style.fontWeight = "600";
  hero.style.transition = "opacity 0.5s ease";
  hero.style.opacity = "1";
  hero.style.zIndex = "9999";
  document.body.appendChild(hero);

  setTimeout(() => {
    hero.style.opacity = "0";
    setTimeout(() => {
      document.body.removeChild(hero);
    }, 500);
  }, 3000);
}
