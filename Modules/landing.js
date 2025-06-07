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

document.addEventListener("DOMContentLoaded", () => {
  let departure, destination, departDate, returnDate;

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

  // Set date limits
  const today = new Date().toISOString().split("T")[0];
  document.querySelectorAll('input[type="date"]').forEach((input) => {
    input.min = today;
  });

  const dDate = document.getElementById("departDate");
  const aDate = document.getElementById("returnDate");

  dDate.addEventListener("change", () => {
    const depDate = dDate.value;
    aDate.min = depDate;
  });

  form.addEventListener("submit", function (e) {
    e.preventDefault();
    resultsDiv.innerHTML = "";
    allFlights = [];

    departure = document.getElementById("departure").value.trim();
    destination = document.getElementById("destination").value.trim();
    departDate = document.getElementById("departDate").value;
    returnDate = document.getElementById("returnDate").value;

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

    try {
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
        console.log(`round to: ${to}`);
        console.log(`round fro: ${fro}`);

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
      }
      window.location.href = "#flights-div";
    } catch (err) {
      console.error("Error during flight search:", err);
      resultsDiv.innerHTML = "<p>Error parsing flight data.</p>";
    }
  });

  document
    .getElementById("book-flight")
    .addEventListener("click", function (e) {
      if (!departure || !destination || !departDate) {
        showHero("Please fill all required fields.", "rgb(250, 215, 61)");
        window.location.href = "#hero";
        return;
      }
    });
});

// Render flights and attach filters
function renderFlightResults(jsonStr, title, departure, destination) {
  const flights = JSON.parse(jsonStr);
  allFlights = allFlights.concat(flights); // Accumulate for both directions

  const resultsDiv = document.querySelector(".searched-flights");

  // Clear previous if not already filtered
  if (!document.querySelector(".flight-filters")) {
    resultsDiv.innerHTML = ""; // Clear only once
    initializeFilterUI();
  }

  // Save titles (to distinguish in filtering)
  flights.forEach((f) => (f.__title = title));

  applyFilters();
}

// Dynamically insert checkbox filters
function initializeFilterUI() {
  const filterSection = document.createElement("div");
  filterSection.className = "flight-filters";

  const filters = [
    { id: "morning", label: "Morning Flights (5AM - 12PM)" },
    { id: "evening", label: "Evening Flights (5PM - 11PM)" },
    { id: "nonstop", label: "Non-Stop Flights" },
    { id: "onestop", label: "One Stop" },
    { id: "twostop", label: "Two Stops" },
  ];

  filters.forEach(({ id, label }) => {
    const wrapper = document.createElement("div");
    wrapper.style.marginBottom = "4px";

    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.id = id;
    checkbox.name = id;

    checkbox.addEventListener("change", applyFilters);

    const labelEl = document.createElement("label");
    labelEl.htmlFor = id;
    labelEl.textContent = label;

    wrapper.appendChild(checkbox);
    wrapper.appendChild(labelEl);
    filterSection.appendChild(wrapper);
  });

  const resultsDiv = document.querySelector(".searched-flights");
  resultsDiv.appendChild(filterSection);
}

// Filter and re-render flights
function applyFilters() {
  const resultsDiv = document.querySelector(".searched-flights");

  // Remove all flight cards and titles
  const oldCards = resultsDiv.querySelectorAll(
    ".flight-card, h2, .no-flight-display"
  );
  oldCards.forEach((el) => el.remove());

  const morning = document.getElementById("morning").checked;
  const evening = document.getElementById("evening").checked;
  const nonstop = document.getElementById("nonstop").checked;
  const onestop = document.getElementById("onestop").checked;
  const twostop = document.getElementById("twostop").checked;

  const grouped = {};

  allFlights.forEach((flight) => {
    let show = true;
    const depHour = parseInt(flight.Dtime.split(":")[0]);

    if (morning && (depHour < 5 || depHour >= 12)) show = false;
    if (evening && (depHour < 17 || depHour > 23)) show = false;

    const stopCount = flight.stops?.length || 0;
    if (nonstop && stopCount !== 0) show = false;
    if (onestop && stopCount !== 1) show = false;
    if (twostop && stopCount !== 2) show = false;

    if (show) {
      if (!grouped[flight.__title]) grouped[flight.__title] = [];
      grouped[flight.__title].push(flight);
    }
  });

  const titles = Object.keys(grouped);
  if (titles.length === 0) {
    const noMatch = document.createElement("p");
    noMatch.className = "no-flight-display";
    noMatch.textContent = "No flight matches the selected filters.";
    resultsDiv.appendChild(noMatch);
    return;
  }

  titles.forEach((title) => {
    const sectionTitle = document.createElement("h2");
    sectionTitle.className = "title";
    sectionTitle.textContent = title;
    resultsDiv.appendChild(sectionTitle);

    grouped[title].forEach((flight) => {
      const card = document.createElement("div");
      card.className = "flight-card";

      const imgDiv = document.createElement("div");
      imgDiv.className = "flight-image";
      const img = document.createElement("img");
      img.src = "/images/fl.jpg";
      img.alt = "Flight";
      imgDiv.appendChild(img);

      const detailsDiv = document.createElement("div");
      detailsDiv.className = "flight-details";

      const rowDiv = document.createElement("div");
      rowDiv.className = "flight-row";

      const flightNumber = document.createElement("span");
      flightNumber.className = "flight-number";
      flightNumber.textContent = flight.flightNumber;

      const route = document.createElement("span");
      route.textContent = `${flight.departure} → ${flight.destination}`;

      const travelTime = document.createElement("span");
      travelTime.className = "travel-time";
      travelTime.textContent = `${flight.departDate} - ${flight.Dtime}`;

      const price = document.createElement("span");
      price.className = "price";
      price.textContent = `₹${flight.price}`;

      rowDiv.append(flightNumber, route, travelTime, price);

      const datesDiv = document.createElement("div");
      datesDiv.className = "flight-dates";

      const stops = document.createElement("small");
      stops.innerHTML = `Stops: ${
        flight.stops && flight.stops.length > 0
          ? flight.stops.map((stop) => `<span>${stop}</span>`).join(", ")
          : "None"
      }`;

      const arrival = document.createElement("small");
      arrival.textContent = `Arrives: ${flight.arrivalDate} → ${flight.Atime}`;

      const status = document.createElement("small");
      status.textContent = `Status: ${flight.status}`;

      const seats = document.createElement("small");
      seats.style.backgroundColor = flight.seats > 2 ? "green" : "red";
      seats.style.color = "white";
      seats.style.padding = "8px";
      seats.style.borderRadius = "10px";
      seats.textContent = `Seats Available: ${
        flight.seats > 0 ? flight.seats : "full"
      }`;

      datesDiv.append(stops, arrival, seats, status);
      datesDiv.style.marginTop = "10px";
      detailsDiv.append(rowDiv, datesDiv);

      const buttonDiv = document.createElement("div");
      const bookBtn = document.createElement("button");
      bookBtn.className = "book";
      bookBtn.textContent = flight.seats > 0 ? "Book Flight" : "Flight Full";

      if (flight.seats > 0) {
        bookBtn.addEventListener("click", () => {
          localStorage.setItem("selectedFlight", JSON.stringify(flight));
          window.location.href = "./bookflight.html";
        });
      } else {
        bookBtn.disabled;
      }

      buttonDiv.appendChild(bookBtn);
      card.append(imgDiv, detailsDiv, buttonDiv);
      resultsDiv.appendChild(card);
    });
  });
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
