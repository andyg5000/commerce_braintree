<?php

/**
 * @file
 * Include payment method settings callback, redirect form callbacks...
 */

/**
 * Payment method callback: settings form.
 */
function commerce_braintree_settings_form($settings = NULL) {
  $form = commerce_braintree_default_settings_form($settings);
  return $form;
}

/**
 * Payment method callback: redirect form.
 *
 * Building the request that is going to be sent to braintree after the
 * necessary checks.
 */
function commerce_braintree_redirect_form($form, &$form_state, $order, $payment_method) {
  $settings = _commerce_braintree_set_feedback_url($order, $payment_method);
  global $user;

  list($amount, $customer_name, $country, $thoroughfare, $locality, $postal_code, $administrative_area, $customer_mail) = _commerce_braintree_get_transaction_informations($order);

  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  _commerce_braintree_init_credentials($payment_method);
  $form = commerce_braintree_credit_card_form($payment_method);

  // Retrieve the order balance instead of the order total, this allows you
  // to pay your order with multiple payment methods.
  $balance = commerce_payment_order_balance($order);
  $amount = commerce_currency_amount_to_decimal($balance['amount'], commerce_default_currency());
  $trData = Braintree_TransparentRedirect::transactionData(
    array(
      // Add transaction related data.
      'transaction' => array(
        'type' => Braintree_Transaction::SALE,
        'amount' => $amount,
        'customer' => array(
          'firstName' => $customer_name,
          'email' => $customer_mail,
        ),
        'billing' => array(
          'countryCodeAlpha2' => $country,
          'streetAddress' => $thoroughfare,
          'firstName' => $customer_name,
          'locality' => $locality,
          'postalCode' => $postal_code,
          'region' => $administrative_area,
        ),
        'options' => array(
          'submitForSettlement' => TRUE
        ),
      ),

      'redirectUrl' => url('checkout/' . $order->order_id . '/payment/return/' . $order->data['payment_redirect_key'], array('absolute' => TRUE))
    )
  );

  $form['tr_data']['#type'] = 'hidden';
  $form['tr_data']['#name'] = 'tr_data';
  $form['tr_data']['#default_value'] = $trData;
  // Change the action. The form will be submitted directly to braintree's
  // servers.
  $form['#action'] = Braintree_TransparentRedirect::url();
  $form['submit'] = array('#type' => 'submit', '#value' => t('submit'));
  return $form;
}

/**
 * Implements hook_redirect_form_validate().
 */
function commerce_braintree_redirect_form_validate($order, $payment_method) {
  if ($feedback = commerce_braintree_get_feedback()) {
    // Process the transaction based on the parameters received.
    commerce_braintree_process_transaction($order, $payment_method, $feedback);
    return TRUE;
  }
}

/**
 * Process the payment transaction with the info received.
 *
 * @param object $order
 *   The loaded order that is being processed
 * @param array $payment_method
 *   The payment method settings
 * @param string $feedback
 *   The parameters received from Braintree regarding the payment
 * @param bool $redirect
 *   Specifies whether to call redirect functions or not
 */
function commerce_braintree_process_transaction($order, $payment_method, $feedback, $redirect = TRUE) {

  _commerce_braintree_init_credentials($payment_method);
  $result = Braintree_TransparentRedirect::confirm($feedback);

  _commerce_braintree_default_process_transaction($result, $order, $payment_method, $redirect);
}
