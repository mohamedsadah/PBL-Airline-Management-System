window.addEventListener("DOMContentLoaded", () => {
  const navMenu = document.querySelector(".nav-menu");
  const isLoggedIn = localStorage.getItem("isLoggedIn") === "true";

  if (isLoggedIn) {
    const dashboardLink = document.createElement("li");
    dashboardLink.innerHTML = `<a href="./userDashboard.html">View Dashboard</a>`;
    dashboardLink.id = "dashboardMenu";

    const logoutLink = document.createElement("li");
    logoutLink.innerHTML = `<a href="#" id="logoutBtn">Logout</a>`;
    logoutLink.id = "logoutMenu";

    navMenu.appendChild(dashboardLink);
    navMenu.appendChild(logoutLink);

    // Remove login link if present
    const loginItem = navMenu.querySelector('a[href="./login.html"]');
    if (loginItem) loginItem.parentElement.remove();
  }

  // Handle logout
  const logoutBtn = document.getElementById("logoutBtn");
  if (logoutBtn) {
    logoutBtn.addEventListener("click", () => {
      localStorage.removeItem("isLoggedIn");
      localStorage.removeItem("loggedInUser");
      window.location.href = "./landing.html";
    });
  }

  if (document.body.hasAttribute("data-auth-required") && !user) {
    window.location.href = "./login.html";
  }
});
