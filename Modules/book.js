let save;
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

      Module.ccall("loadPassengers", null, [], []);

      const ptr = Module.ccall("getAllPassengersJSON", "number", []);
      const Pjson = Module.UTF8ToString(ptr);
      passengers = JSON.parse(Pjson);
      console.log(passengers);

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
                        <span class="travel-time">${flight.departDate} -> ${
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
                        <small>Arrives: ${flight.arrivalDate} - ${
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
            gatewayMerchantId: "exampleMerchantId",
          },
        },
      },
    ],
    merchantInfo: {
      merchantName: "Test Merchant",
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

      if (!/^\d+$/.test(contact)) {
        alert("Contact number should contain digits only.");
        return;
      }

      let found = 0;

      const booked = passengers.forEach((p) => {
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

  // Load banner image
  const bannerImg = document.getElementById("bg");
  const bannerCanvas = document.createElement("canvas");
  bannerCanvas.width = bannerImg.width;
  bannerCanvas.height = bannerImg.height;
  const bctx = bannerCanvas.getContext("2d");
  bctx.drawImage(bannerImg, 0, 0);

  // Apply dark overlay
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

  // Logo setup
  const logo = document.getElementById("logo");
  const logoCanvas = document.createElement("canvas");
  logoCanvas.width = logo.width;
  logoCanvas.height = logo.height;
  const lctx = logoCanvas.getContext("2d");
  lctx.drawImage(logo, 0, 0);
  const logoData = logoCanvas.toDataURL("image/png");

  doc.addImage(logoData, "PNG", 10, 5, 15, 15); // Logo inside banner

  // Title text on top of banner
  doc.setFont("helvetica", "bold");
  doc.setTextColor(255, 255, 255);
  doc.setFontSize(16);
  doc.text("Skycoders Airline", 30, 15);
  doc.setFontSize(12);
  doc.text("Flight Itinerary", pageWidth - 50, 15);

  // White card with rounded corners
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

  // Coordinate columns inside the card
  const leftX = cardX + paddingX;
  const midX = leftX + 65;
  const rightX = leftX + 130;

  // Passenger Info
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

  // Section divider
  y += 5;
  doc.setDrawColor(220);
  doc.line(leftX, y, pageWidth - paddingX, y);
  y += 10;

  // Flight Info
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
  doc.text("Departure:", leftX, y);
  doc.text("Arrival:", midX, y);
  doc.text("Stops:", rightX, y);

  y += 7;
  doc.setFont("helvetica", "normal");
  doc.text(flight.departDate || "-", leftX, y);
  doc.text(flight.arrivalDate || "-", midX, y);
  doc.text(flight.stopCount?.toString() || "-", rightX, y);

  if (flight.stops && flight.stops.length > 0) {
    y += 10; // Add vertical space after last row
    doc.setFont("helvetica", "bold");
    doc.text("Flight Stops:", leftX, y);

    doc.setFont("helvetica", "normal");
    const stopsText = flight.stops.join(", ");
    const splitStops = doc.splitTextToSize(stopsText, pageWidth - 2 * leftX);
    y += 6;
    doc.text(splitStops, leftX, y);
    y += splitStops.length * 6; // Adjust y for remaining sections
  } else {
    y += 10;
    doc.setFont("helvetica", "bold");
    doc.text("Flight Stops:", leftX, y);
    doc.setFont("helvetica", "normal");
    doc.text("None", leftX + 30, y);
  }
  y -= 12;
  doc.setFont("helvetica", "bold");
  doc.text("Departure Time:", midX, y);
  doc.text("Arrival Time:", rightX, y);

  y += 7;
  doc.setFont("helvetica", "normal");
  doc.setFont("helvetica", "normal");
  doc.text(flight.Dtime || "-", midX, y);
  doc.text(flight.Atime || "-", rightX, y);

  y += 10;
  doc.setFont("helvetica", "bold");
  doc.text("Fare (INR):", leftX, y);

  y += 7;
  doc.setFont("helvetica", "normal");
  doc.text(`₹${flight.price}`, leftX, y);

  // QR Code
  y += 15;
  const qrText = `Flight: ${flight.flightNumber}, Name: ${user.name}, Passport: ${user.passport}`;
  QRCode.toDataURL(qrText).then((qrDataURL) => {
    doc.setFont("helvetica", "bold");
    doc.text("Verification QR Code:", leftX, y);
    doc.addImage(qrDataURL, "PNG", leftX, y + 5, 40, 40);

    // Luggage Info (outside card if needed)
    doc.setFont("helvetica", "bold");
    doc.setTextColor(255, 255, 255);
    doc.text("Luggage & Baggage Policy", 10, cardY + cardHeight + 15);
    doc.setFont("helvetica", "bold");
    doc.setFontSize(11);
    doc.text(
      "Cabin Bag: 7kg | Check-in: 20kg | Extra Baggage: 500rps/kg",
      10,
      cardY + cardHeight + 22
    );
    doc.text(
      "Note: Reach airport at least 2 hours before departure time.",
      10,
      cardY + cardHeight + 30
    );

    // Footer
    doc.setFontSize(10);
    doc.setTextColor(255, 255, 255);
    doc.text(
      "Thank you for flying with Skycoders Airline. Safe travels!",
      10,
      pageHeight - 15
    );
    doc.text("© Skycoders Airline 2025", pageWidth - 50, pageHeight - 10);

    doc.save("Skycoders-Itinerary.pdf");
  });
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

  generateItinerary(user, flight);

  const result = save(
    uName,
    passNumber,
    flight.flightNumber,
    nationality,
    contact
  );

  //debug
  console.log(result);
}
