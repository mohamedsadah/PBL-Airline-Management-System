const express = require('express');
const router = express.Router();
const { createOrder, verifyPayment } = require('./paymentController');

// Route to create payment order
router.post('/create-order', createOrder);

// Route to verify payment success
router.post('/verify-payment', verifyPayment);

module.exports = router;
