<?php


/**
 * Payment method callback: settings form.
 *
 *
 * @DEV : http://d7.dev/#overlay=admin/commerce/config/payment-methods/manage/commerce_payment_braintree/edit/3
 */
function commerce_braintree_settings_form($settings = NULL) {
  $form = array();
  $default_currency = commerce_default_currency();
  // Merge default settings into the stored settings array.
  // Demonstration or production ?
  $form['commerce_braintree_mode'] = array(
    '#type' => 'select',
    '#title' => t('Mode'),
    '#options' => array(
      'sandbox' => t('Sandbox'),
      'production' => t('Production'),
    ),
    '#default_value' => $settings['commerce_braintree_mode'],
  );
  $form['commerce_braintree_merchant_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Merchant ID'),
    '#description' => t("The merchantID."),
    '#size' => 30,
    '#maxlength' => 128,
    '#default_value' => $settings['commerce_braintree_merchant_id'],
    '#required' => TRUE,
  );
  $form['commerce_braintree_public_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Public Key'),
    '#description' => t("The publicKey."),
    '#size' => 30,
    '#maxlength' => 128,
    '#default_value' => $settings['commerce_braintree_public_key'],
    '#required' => TRUE,
  );
  $form['commerce_braintree_private_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Private Key'),
    '#description' => t("The privateKey."),
    '#size' => 40,
    '#maxlength' => 128,
    '#default_value' => $settings['commerce_braintree_private_key'],
    '#required' => TRUE,
  );

  return $form;
}

/**
 * Payment method callback: redirect form, we're building the request that is
 * going to be sent to atos after the necessary checks.
 */
function commerce_braintree_redirect_form($form, &$form_state, $order, $payment_method) {
  $queryString = $_SERVER['QUERY_STRING'];
  if (!empty($queryString)) {
    require_once drupal_get_path('module', 'commerce_braintree') . '/braintree_php/lib/Braintree.php';


  Braintree_Configuration::environment($payment_method['settings']['commerce_braintree_mode']);
  Braintree_Configuration::merchantId($payment_method['settings']['commerce_braintree_merchant_id']);
  Braintree_Configuration::publicKey($payment_method['settings']['commerce_braintree_public_key']);
  Braintree_Configuration::privateKey($payment_method['settings']['commerce_braintree_private_key']);
    $result = Braintree_TransparentRedirect::confirm($queryString);
    dsm($result);
    if ($result->success) {
      commerce_payment_redirect_pane_next_page($order);
      dvm($result->transaction->status);
    }
    else {
      dsm($result->errors->deepAll());
    }
  }
  else {
    module_load_include('inc', 'commerce_payment', 'includes/commerce_payment.credit_card');
    require_once drupal_get_path('module', 'commerce_braintree') . '/braintree_php/lib/Braintree.php';

    Braintree_Configuration::environment($payment_method['settings']['commerce_braintree_mode']);
    Braintree_Configuration::merchantId($payment_method['settings']['commerce_braintree_merchant_id']);
    Braintree_Configuration::publicKey($payment_method['settings']['commerce_braintree_public_key']);
    Braintree_Configuration::privateKey($payment_method['settings']['commerce_braintree_private_key']);
    $form = array();

    $form['transaction']['credit_card']['number'] = array(
      '#type' => 'textfield',
      '#default_value' => '4111111111111111',
      '#name' => 'transaction[credit_card][number]'
    );
    $form['transaction']['credit_card']['expiration_date'] = array(
      '#type' => 'textfield',
      '#default_value' => '12/2015',
      '#name' => 'transaction[credit_card][expiration_date]'
    );

    // Retrieve the order balance instead of the order total, this allows you
    // to pay your order with multiple payment methods.
    $balance = commerce_payment_order_balance($order);
    $amount = commerce_currency_amount_to_decimal($balance['amount'], commerce_default_currency());;
    $trData = Braintree_TransparentRedirect::transactionData(
      array(
        'transaction' => array(
          'type' => Braintree_Transaction::SALE,
          'amount' => $amount,
        ),

        'redirectUrl' => url('checkout/' . $order->order_id . '/payment/return', array('absolute' => TRUE))
      )
    );
    $form['tr_data']['#type'] = 'hidden';
    $form['tr_data']['#name'] = 'tr_data';
    $form['tr_data']['#default_value'] = $trData;
    $form['#action'] = Braintree_TransparentRedirect::url();
    $form['submit'] = array('#type' => 'submit', '#value' => t('submit'));
    return $form;
  }
}
