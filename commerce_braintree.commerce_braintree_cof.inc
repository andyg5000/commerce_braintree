<?php

/**
 * @file
 * Include payment method settings callback, redirect form callbacks...
 */

/**
 * Payment method callback: settings form.
 */
function commerce_braintree_cof_settings_form($settings = NULL) {
  $form = commerce_braintree_default_settings_form($settings);
  return $form;
}

/**
 * Payment method callback: redirect form.
 *
 * Building the request that is going to be sent to braintree after the
 * necessary checks.
 */
function commerce_braintree_cof_redirect_form($form, &$form_state, $order, $payment_method) {
  global $user;

  $context = commerce_braintree_payment_session_load($order->order_id);
  $settings = _commerce_braintree_set_feedback_url($order, $payment_method);

  list($amount, $customer_name, $country, $thoroughfare, $locality, $postal_code, $administrative_area, $customer_mail) = _commerce_braintree_get_transaction_informations($order);

  module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
  _commerce_braintree_init_credentials($payment_method);

  // Retrieve the order balance instead of the order total, this allows you
  // to pay your order with multiple payment methods.
  $balance = commerce_payment_order_balance($order);
  $amount = commerce_currency_amount_to_decimal($balance['amount'], commerce_default_currency());
  if ($context === NULL || $context == 'new') {
    $form = commerce_braintree_credit_card_form($payment_method);
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
            'storeInVault' => TRUE,
            'submitForSettlement' => TRUE,
          ),
        ),
        'redirectUrl' => url('checkout/' . $order->order_id . '/payment/return/' . $order->data['payment_redirect_key'], array('absolute' => TRUE))
      )
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('submit'),
      '#suffix' => l(t('Cancel'), 'checkout/' . $order->order_id . '/payment/back/' . $order->data['payment_redirect_key'], array('absolute' => TRUE)),
    );
  }
  else {
    global $user;
    $stored_cards = commerce_cardonfile_data_load_multiple($user->uid, $payment_method['instance_id']);
    $element = variable_get('commerce_cardonfile_selector', 'radios');
    $options = commerce_cardonfile_options_list($stored_cards, $element);
    $card = commerce_braintree_load_cardonfile_data($user->uid, commerce_braintree_payment_session_load($order->order_id));
    $settings = _commerce_braintree_set_feedback_url($order, $payment_method);

    $trData = Braintree_TransparentRedirect::transactionData(
      array(
        // Add transaction related data.
        'transaction' => array(
          'type' => Braintree_Transaction::SALE,
          'amount' => $amount,
          'paymentMethodToken' => $card['remote_id'],
        ),
        'redirectUrl' => url('checkout/' . $order->order_id . '/payment/return/' . $order->data['payment_redirect_key'], array('absolute' => TRUE))
      )
    );
    $form['card']['#markup'] = theme('card_data_overview', array('card_data' => commerce_cardonfile_data_load($card)));
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Proceed to payment'),
      '#suffix' => l(t('Cancel'), 'checkout/' . $order->order_id . '/payment/back/' . $order->data['payment_redirect_key'], array('absolute' => TRUE)),
    );
  }
  $form['tr_data']['#type'] = 'hidden';
  $form['tr_data']['#name'] = 'tr_data';
  $form['tr_data']['#default_value'] = $trData;
  // Change the action. The form will be submitted directly to braintree's
  // servers.
  $form['#action'] = Braintree_TransparentRedirect::url();
  return $form;
}

/**
 * Implements hook_redirect_form_validate().
 */
function commerce_braintree_cof_redirect_form_validate($order, $payment_method) {
  if ($feedback = commerce_braintree_get_feedback()) {
    // Process the transaction based on the parameters received.
    commerce_braintree_cof_process_transaction($order, $payment_method, $feedback);
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
function commerce_braintree_cof_process_transaction($order, $payment_method, $feedback, $redirect = TRUE) {

  _commerce_braintree_init_credentials($payment_method);
  $result = Braintree_TransparentRedirect::confirm($feedback);
  $context = commerce_braintree_payment_session_load($order->order_id);
  if (($context === NULL || $context == 'new') && $result->success) {
    // Card on file parameters.
    $new_card_data = array();
    // uid: the user ID of the account the card data is being stored for.
    $new_card_data['uid'] = $order->uid;
    // payment_method: the name of the payment method the card was used for.
    $new_card_data['payment_method'] = $payment_method['method_id'];
    // instance_id: the payment method instance ID containing the credentials
    // that will be used to reuse the card on file.
    $new_card_data['instance_id'] = $payment_method['instance_id'];
    // remote_id: the remote ID to the full card data at the payment gateway.
    $new_card_data['remote_id'] = $result->transaction->_attributes['creditCard']['token'];
    // card_type: short name of the credit card type if determined, based on the
    // keys returned by commerce_payment_credit_card_types().
    $new_card_data['card_type'] = $result->transaction->_attributes['creditCard']['cardType'];
    // card_name: the name of the cardholder.
    $new_card_data['card_name'] = $result->transaction->_attributes['creditCard']['cardholderName'];
    // card_number: the last 4 digits of the credit card number.
    $new_card_data['card_number'] = $result->transaction->_attributes['creditCard']['last4'];
    // card_exp_month: the numeric representation of the expiration month.
    $new_card_data['card_exp_month'] = $result->transaction->_attributes['creditCard']['expirationMonth'];
    // card_exp_year: the four digit expiration year.
    $new_card_data['card_exp_year'] = $result->transaction->_attributes['creditCard']['expirationYear'];
    // status: integer status of the card data: inactive (0), active (1), or
    // active and not deletable (2).
    $new_card_data['status'] = 1;
    // Save and log the creation of the new card on file.
    $save = commerce_cardonfile_data_save($new_card_data);
    watchdog('commerce_braintree', 'COF, with remote ID @profile_id, added for user @uid.', array(
      '@profile_id' => $new_card_data['remote_id'],
      '@uid' => $order->uid
    ));
  }
  commerce_braintree_payement_session_delete($order->order_id);
  _commerce_braintree_default_process_transaction($result, $order, $payment_method, $redirect);
}

/**
 * Loads stored card data by user and ID.
 *
 * @param int $uid
 *   The user ID of the user's stored card data to load.
 * @param int $card_id
 *   The local ID of the stored card data to load.
 *
 * @return array
 *   An array containing the specified card data or FALSE if the specified card
 *   data does not exist.
 */
function commerce_braintree_load_cardonfile_data($uid, $card_id) {
  return db_select('commerce_card_data', 'ccd')
    ->fields('ccd')
    ->condition('ccd.uid', $uid)
    ->condition('ccd.card_id', $card_id)
    ->execute()
    ->fetchAssoc();
}
