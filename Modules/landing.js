const images = [
    'url("/images/fl.jpg")',
    'url("/images/fl3.jpg")',
    'url("/images/flight.jpg")',
    'url("/images/fl4.jpg")',
    'url("/images/fl5.jpg")',
    'url("/images/fl6.jpg")',
    'url("/images/fl7.jpg")',
    'url("/images/fl8.jpg")'
  ];
  
  let currentIndex = 0;
  const landingSection = document.querySelector('.landing-section');
  
  function changeBackground() {
    currentIndex = (currentIndex + 1) % images.length;
    landingSection.style.backgroundImage = images[currentIndex];
  }
  
  setInterval(changeBackground, 3000); // every 3 seconds
  
  // Fade-in info section when in view
  window.addEventListener('scroll', () => {
    const info = document.querySelector('.info-section');
    const rect = info.getBoundingClientRect();
    if (rect.top < window.innerHeight - 100) {
      info.classList.add('visible');
    }
  });
  