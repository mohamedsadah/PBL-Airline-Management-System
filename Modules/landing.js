// Emscripten Module setup
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

      // Register C-wrapped functions globally
      searchOneWay = Module.cwrap("getOneWayFlightsJSON", "string", [
        "string",
        "string",
        "string",
      ]);
    });
  },
};

let searchOneWay, searchRoundTrip;

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

    const departure = document.getElementById("departure").value.trim();
    const destination = document.getElementById("destination").value.trim();
    const departDate = document.getElementById("departDate").value;
    const returnDate = document.getElementById("returnDate").value;
    const tripType = document.querySelector(
      'input[name="trip"]:checked'
    )?.value;

    if (!departure || !destination || !departDate) {
      alert("Please fill all required fields.");
      return;
    }

    if (tripType === "round" && !returnDate) {
      alert("Please select return date for round trip.");
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
          `Flights from ${destination} to ${departure}`,
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
          <span class="travel-time">${flight.time}</span>
          <span class="price">₹${flight.price}</span>
      </div>
      <div class="flight-dates">
          <small>No. of Stops: ${flight.stopCount}</small>
          <small>Stops: ${
            flight.stops && flight.stops.length > 0
              ? flight.stops.map((stop) => `<span>${stop}</span>`).join(", ")
              : "None"
          }</small>
          <div></div>
          <small>${flight.departDate} → ${flight.arrivalDate}</small>
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

// Background slideshow
const images = [
  'url("/images/fl.jpg")',
  'url("/images/fl3.jpg")',
  'url("/images/flight.jpg")',
  'url("/images/f4.jpg")',
  'url("/images/f5.jpg")',
  'url("/images/f6.jpg")',
  'url("/images/f7.jpg")',
  'url("/images/fl8.jpg")',
];
let currentIndex = 0;
const landingSection = document.querySelector(".hero-section");
function changeBackground() {
  currentIndex = (currentIndex + 1) % images.length;
  landingSection.style.backgroundImage = images[currentIndex];
}
setInterval(changeBackground, 3000);

// Scroll-in animation
window.addEventListener("scroll", () => {
  const info = document.querySelector(".info-section");
  const rect = info.getBoundingClientRect();
  if (rect.top < window.innerHeight - 100) {
    info.classList.add("visible");
  }
});
