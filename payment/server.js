const express = require('express');
const dotenv = require('dotenv');
const paymentRoutes = require('./paymentRoutes');
const cors = require('cors');




dotenv.config(); // Load environment variables from .env

const app = express();

app.use(cors());
// Middleware to parse JSON request bodies
app.use(express.json());

// Register the payment routes
app.use('/api/payment', paymentRoutes);

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}`);
});
