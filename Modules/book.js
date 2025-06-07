let save, createRecord;
let passengers = [];

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

      const generated = localStorage.getItem("seatsGenerator") === "true";
      Module.ccall("loadPassengers", null, [], []);
      Module.ccall("loadFlightsFromFile", null, [], []);

      // if (!generated) Module.ccall("seatsGenerator", null, [], []);
      localStorage.setItem("seatsGenerator", true);

      const ptr = Module.ccall("getAllPassengersJSON", "number", []);
      const Pjson = Module.UTF8ToString(ptr);
      passengers = JSON.parse(Pjson);
      console.log(passengers);

      createRecord = Module.cwrap("createPassengerRecords", "number", [
        "string",
        "string",
        "string",
        "string",
        "string",
        "string",
        "string",
        "string",
        "string",
        "number",
        "number",
        "number",
        "number",
      ]);

      save = Module.cwrap("addPassenger", "number", [
        "string",
        "string",
        "string",
        "string",
        "string",
      ]);
    });
  },
};

const flight = JSON.parse(localStorage.getItem("selectedFlight"));

const isLoggedIn = localStorage.getItem("isLoggedIn") === "true";
const userpass = localStorage.getItem("userpass");
const userEmail = localStorage.getItem("loggedInUser");
if (isLoggedIn) document.getElementById("email").value = userEmail;
if (userpass) document.getElementById("passport").value = userpass;

const stopList = flight.stops;
stopList.forEach((s) => console.log(s));

if (flight) {
  document.getElementById(
    "flight-summary"
  ).innerHTML = `<div class="flight-image">
                    <img src="/images/fl.jpg" alt="Flight">
                </div>
                <div class="flight-details">
                    <div class="flight-row">
                        <span class="flight-number">${
                          flight.flightNumber
                        }</span>
                        <span>${flight.departure} → ${flight.destination}</span>
                        <span class="travel-time">${flight.departDate} → ${
    flight.Dtime
  }</span>
                        <span class="price">₹${flight.price}</span>
                    </div>
                    <div class="flight-dates">
                        <small>Stops: ${
                          flight.stops && flight.stops.length > 0
                            ? flight.stops
                                .map((stop) => `<span>${stop}</span>`)
                                .join(", ")
                            : "None"
                        }</small>
                        <small>Arrives: ${flight.arrivalDate} →  ${
    flight.Atime
  }</small>
                        <small>Status: ${flight.status}</small>
                    </div>
                </div>
                <div>
                </div>`;
}

function onGooglePayLoaded() {
  const paymentsClient = new google.payments.api.PaymentsClient({
    environment: "TEST",
  });

  const paymentDataRequest = {
    apiVersion: 2,
    apiVersionMinor: 0,
    allowedPaymentMethods: [
      {
        type: "CARD",
        parameters: {
          allowedAuthMethods: ["PAN_ONLY", "CRYPTOGRAM_3DS"],
          allowedCardNetworks: ["MASTERCARD", "VISA"],
        },
        tokenizationSpecification: {
          type: "PAYMENT_GATEWAY",
          parameters: {
            gateway: "example",
            gatewayMerchantId: "01234567890123456789",
          },
        },
      },
    ],
    merchantInfo: {
      merchantName: "SkyCoders Airline",
    },
    transactionInfo: {
      totalPriceStatus: "FINAL",
      totalPrice: `${flight.price}`,
      currencyCode: "INR",
      countryCode: "IN",
    },
  };

  function pay() {
    const paymentsClient = new google.payments.api.PaymentsClient({
      environment: "TEST",
    });

    paymentsClient
      .loadPaymentData(paymentDataRequest)
      .then((paymentData) => {
        console.log("Payment Success", paymentData);
        showHero("Payment Successful!");
        handlePaymentSuccess();

        setTimeout(() => {
          window.location.href = "/Modules/landing.html";
        }, 5000);
      })
      .catch((err) => {
        console.error("Payment Error", err);
        showHero("Payment Failed");
      });
  }

  document
    .getElementById("gpay-button")
    .addEventListener("click", function (e) {
      e.preventDefault();

      const userName = document.getElementById("name").value.trim();
      const passportNumber = document.getElementById("passport").value.trim();
      const nationality = document.getElementById("nationality").value.trim();
      const contact = document.getElementById("contact").value.trim();

      if (!userName || !passportNumber || !nationality || !contact) {
        alert("Please fill in all booking information fields.");
        return;
      }

      const pattern = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

      if (isNaN(contact)) {
        showHero("contact should contain digits only");
        return;
      }

      let found = 0;

      passengers.forEach((p) => {
        if (
          p.PassportNumber == passportNumber &&
          p.flightNumber == flight.flightNumber
        ) {
          found = 1;
        }
      });

      console.log(found);

      if (found) {
        showHero("You have already booked this flight");
      } else {
        pay();
      }
    });
}

document
  .getElementById("cancel-button")
  .addEventListener("click", function (e) {
    window.location.href = "/Modules/landing.html";
  });

function showHero(Message) {
  const hero = document.createElement("div");
  hero.textContent = Message;
  hero.style.position = "fixed";
  hero.style.top = "20px";
  hero.style.right = "50px";
  if (Message != "Payment Successful!") {
    hero.style.backgroundColor = "red";
  } else {
    hero.style.backgroundColor = "#4BB543";
  }
  hero.style.color = "#fff";
  hero.style.padding = "15px 25px";
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
  }, 5000);
}

//create itenerary

async function generateItinerary(user, flight) {
  const { jsPDF } = window.jspdf;
  const doc = new jsPDF("p", "mm", "a4");
  const pageWidth = doc.internal.pageSize.getWidth();
  const pageHeight = doc.internal.pageSize.getHeight();

  // === Banner Setup ===
  const bannerImg = document.getElementById("bg");
  const bannerCanvas = document.createElement("canvas");
  bannerCanvas.width = bannerImg.width;
  bannerCanvas.height = bannerImg.height;
  const bctx = bannerCanvas.getContext("2d");
  bctx.drawImage(bannerImg, 0, 0);
  bctx.fillStyle = "rgba(0, 0, 0, 0.62)";
  bctx.fillRect(0, 0, bannerCanvas.width, bannerCanvas.height);
  const bannerData = bannerCanvas.toDataURL("image/jpeg");

  doc.addImage(
    bannerData,
    "JPEG",
    -10,
    0,
    pageWidth + pageWidth * 0.5,
    pageHeight
  );

  // === Logo ===
  const logo = document.getElementById("logo");
  const logoCanvas = document.createElement("canvas");
  logoCanvas.width = logo.width;
  logoCanvas.height = logo.height;
  const lctx = logoCanvas.getContext("2d");
  lctx.drawImage(logo, 0, 0);
  const logoData = logoCanvas.toDataURL("image/png");
  doc.addImage(logoData, "PNG", 10, 5, 15, 15);

  // === Title Text ===
  doc.setFont("helvetica", "bold");
  doc.setTextColor(255, 255, 255);
  doc.setFontSize(16);
  doc.text("Skycoders Airline", 30, 15);
  doc.setFontSize(12);
  doc.text("Flight Itinerary", pageWidth - 50, 15);

  // === White Card ===
  const cardX = 10;
  const cardY = 35;
  const cardWidth = pageWidth - 20;
  const cardHeight = 210;
  const cardRadius = 3;
  const paddingX = 15;
  let y = cardY + 10;

  doc.setDrawColor(200);
  doc.setFillColor(255, 255, 255);
  doc.roundedRect(
    cardX,
    cardY,
    cardWidth,
    cardHeight,
    cardRadius,
    cardRadius,
    "FD"
  );

  // === Columns ===
  const leftX = cardX + paddingX;
  const midX = leftX + 65;
  const rightX = leftX + 130;

  // === Passenger Info ===
  doc.setFont("helvetica", "bold");
  doc.setTextColor(0, 0, 0);
  doc.setFontSize(12);
  doc.text("Passenger Name:", leftX, y);
  doc.text("Passport No:", midX, y);
  doc.text("Nationality:", rightX, y);
  y += 7;
  doc.setFont("helvetica", "normal");
  doc.text(user.name || "-", leftX, y);
  doc.text(user.passport || "-", midX, y);
  doc.text(user.nationality || "-", rightX, y);

  y += 10;
  doc.setFont("helvetica", "bold");
  doc.text("Contact:", leftX, y);
  doc.setFont("helvetica", "normal");
  doc.text(user.contact || "-", leftX + 25, y);

  // === Divider Line ===
  y += 5;
  doc.setDrawColor(220);
  doc.line(leftX, y, pageWidth - paddingX, y);
  y += 10;

  // === Flight Info ===
  doc.setFont("helvetica", "bold");
  doc.text("Flight No:", leftX, y);
  doc.text("From:", midX, y);
  doc.text("To:", rightX, y);
  y += 7;
  doc.setFont("helvetica", "normal");
  doc.text(flight.flightNumber || "-", leftX, y);
  doc.text(flight.departure || "-", midX, y);
  doc.text(flight.destination || "-", rightX, y);

  y += 10;
  doc.setFont("helvetica", "bold");
  doc.text("Departure Date:", leftX, y);
  doc.text("Arrival Date:", midX, y);
  doc.text("Stops:", rightX, y);
  y += 7;
  doc.setFont("helvetica", "normal");
  doc.text(flight.departDate || "-", leftX, y);
  doc.text(flight.arrivalDate || "-", midX, y);
  doc.text(flight.stopCount?.toString() || "-", rightX, y);

  // === Stops ===
  y += 10;
  doc.setFont("helvetica", "bold");
  doc.text("Flight Stops:", leftX, y);
  doc.setFont("helvetica", "normal");
  if (flight.stops && flight.stops.length > 0) {
    const stopsText = flight.stops.join(", ");
    const wrappedStops = doc.splitTextToSize(
      stopsText,
      cardWidth - 2 * paddingX
    );
    y += 6;
    doc.text(wrappedStops, leftX, y);
    y += wrappedStops.length * 6;
  } else {
    doc.text("None", leftX + 30, y);
    y += 7;
  }

  // === Time Info (Better aligned) ===
  y += 8;
  doc.setFont("helvetica", "bold");
  doc.text("Departure Time:", leftX, y);
  doc.text("Arrival Time:", midX + 10, y);
  y += 7;
  doc.setFont("helvetica", "normal");
  doc.text(flight.Dtime || "-", leftX, y);
  doc.text(flight.Atime || "-", midX + 10, y);

  // === Fare ===
  y += 10;
  doc.setFont("helvetica", "bold");
  doc.text("Fare (INR):", leftX, y);
  y += 7;
  doc.setFont("helvetica", "normal");
  doc.text(`₹${flight.price}`, leftX, y);

  // === QR Code ===
  y += 15;
  const qrText = `Flight: ${flight.flightNumber}, Name: ${user.name}, Passport: ${user.passport}`;
  const qrDataURL = await QRCode.toDataURL(qrText);
  doc.setFont("helvetica", "bold");
  doc.text("Verification QR Code:", leftX, y);
  doc.addImage(qrDataURL, "PNG", leftX, y + 5, 40, 40);

  // === Luggage Info ===
  const luggageY = cardY + cardHeight + 15;
  doc.setFont("helvetica", "bold");
  doc.setTextColor(255, 255, 255);
  doc.text("Luggage & Baggage Policy", 10, luggageY);
  doc.setFontSize(11);
  doc.text(
    "Cabin Bag: 7kg | Check-in: 20kg | Extra Baggage: ₹500/kg",
    10,
    luggageY + 7
  );
  doc.text(
    "Note: Reach airport at least 2 hours before departure time.",
    10,
    luggageY + 14
  );

  // === Footer ===
  doc.setFontSize(10);
  doc.text(
    "Thank you for flying with Skycoders Airline. Safe travels!",
    10,
    pageHeight - 15
  );
  doc.text("© Skycoders Airline 2025", pageWidth - 50, pageHeight - 10);

  // === Save ===
  doc.save("Skycoders-Itinerary.pdf");
}

// Call this function on successful payment
function handlePaymentSuccess() {
  const uName = document.getElementById("name").value;
  const passNumber = document.getElementById("passport").value;
  const nationality = document.getElementById("nationality").value;
  const contact = document.getElementById("contact").value;

  const user = {
    name: uName,
    passport: passNumber,
    nationality: nationality,
    contact: contact,
  };

  if (isLoggedIn) localStorage.setItem("userRec", JSON.stringify(user));

  console.log(isLoggedIn);

  let ptrArray = 0,
    result;
  const stopPtrs = [];

  if (isLoggedIn) {
    const stopCount = flight.stopCount;

    if (stopCount > 0) {
      const stopList = flight.stops;

      // Allocate memory for each stop string

      stopList.forEach((s) => {
        console.log(s);
        const len = lengthBytesUTF8(s) + 1;
        const ptr = Module._malloc(len);
        stringToUTF8(s, ptr, len);
        stopPtrs.push(ptr);
      });

      stopPtrs.forEach((s) => console.log(s));

      // Allocate memory for the array of pointers
      ptrArray = Module._malloc(stopCount * 4); // 4 bytes per pointer
      for (let i = 0; i < stopCount; i++) {
        setValue(ptrArray + i * 4, stopPtrs[i], "i32");
      }
    }

    console.log(ptrArray);

    const pRec = createRecord(
      passNumber,
      flight.flightNumber,
      flight.departure,
      flight.destination,
      flight.departDate,
      flight.arrivalDate,
      flight.Dtime,
      flight.Atime,
      flight.status,
      ptrArray,
      flight.stopCount,
      flight.price,
      flight.seats
    );

    localStorage.setItem("userpass", passNumber);

    result = save(uName, passNumber, flight.flightNumber, nationality, contact);

    console.log(pRec);
    stopPtrs.forEach((ptr) => Module._free(ptr));
    Module._free(ptrArray);
  } else {
    result = save(uName, passNumber, flight.flightNumber, nationality, contact);
  }

  generateItinerary(user, flight);

  //debug
  console.log(result);
}
