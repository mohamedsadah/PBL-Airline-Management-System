const Razorpay = require('razorpay');
const crypto = require('crypto');
const razorpay = new Razorpay({
  key_id: process.env.RAZORPAY_KEY_ID, // from .env
  key_secret: process.env.RAZORPAY_KEY_SECRET, // from .env
});

// Create Order
exports.createOrder = async (req, res) => {
  const { amount } = req.body;

  const options = {
    amount: amount * 100, // amount in paise
    currency: 'INR',
    receipt: `receipt_${new Date().getTime()}`,
  };

  try {
    const order = await razorpay.orders.create(options);
    res.json({
      orderId: order.id,
      amount: order.amount,
      currency: order.currency,
    });
  } catch (err) {
    console.error("Error creating order:", err);
    res.status(500).json({ error: "Order creation failed" });
  }
};

// Verify Payment Signature
exports.verifyPayment = (req, res) => {
  const { paymentId, orderId, signature } = req.body;
  const body = orderId + "|" + paymentId;
  const expectedSignature = crypto.createHmac('sha256', process.env.RAZORPAY_KEY_SECRET)
    .update(body)
    .digest('hex');

  if (expectedSignature === signature) {
    res.json({ message: "Payment Verified" });
  } else {
    res.status(400).json({ error: "Payment verification failed" });
  }
};
