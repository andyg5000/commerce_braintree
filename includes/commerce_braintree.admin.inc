<?php
/**
 * @file
 * Provides admin forms for commerce_braintree.
 */

/**
 * Form callback: allows the user to void a transaction.
 */
function commerce_braintree_void_form($form, &$form_state, $order, $transaction) {
  $form_state['order'] = $order;
  $form_state['transaction'] = $transaction;

  // Load and store the payment method instance for this transaction.
  $payment_method = commerce_payment_method_instance_load($transaction->instance_id);
  $form_state['payment_method'] = $payment_method;

  $amount = commerce_currency_format($transaction->amount, $transaction->currency_code);
  $form['markup'] = array(
    '#markup' => '<strong>' . t('Are you sure that you want to void this transaction?') . '</strong>'
      . '<br /><br />'
      . t('Voiding a transaction will prevent the customer from being charged @amount and void the transaction record.', array('@amount' => $amount)),
  );

  $form = confirm_form($form,
    t('Are you sure that you want to void this transaction?'),
    'admin/commerce/orders/' . $order->order_id . '/payment',
    '',
    t('Void'),
    t('Cancel'),
    'confirm'
  );

  return $form;
}

/**
 * Submit handler: process the void request.
 */
function commerce_braintree_void_form_submit($form, &$form_state) {
  $transaction = $form_state['transaction'];
  $payment_method = commerce_payment_method_instance_load($transaction->instance_id);
  commerce_braintree_initialize($payment_method);

  // Attempt to invoke the Braintree API and process a void transaction on the payment.
  try {
    $result = Braintree_Transaction::void($transaction->remote_id);
  }
  catch (Exception $ex) {
    $result = NULL;
  }

  // Inform the user if there were any issues processing the void.
  if (empty($result) || empty($result->success)) {
    $message = !empty($result->_attributes['message']) ? $result->_attributes['message'] : t('No error provided from Braintree');
    drupal_set_message(t('The payment transaction cannot be voided. @message', array('@message' => $message)), 'error');
  }

  // Inform the user of a successful void and update the payment transaction.
  if (!empty($result) && !empty($result->success)) {
    drupal_set_message(t('Void completed successfully.'));

    // Update the payment transaction values to show the void.
    $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
    $transaction->remote_status = 'voided';
    $transaction->message .= '<br />' . t('Voided: @date', array('@date' => format_date(REQUEST_TIME, 'short')));
    commerce_payment_transaction_save($transaction);
  }

  $form_state['redirect'] = 'admin/commerce/orders/' . $form_state['order']->order_id . '/payment';
}
