<?php
/**
 * @file
 * Provides hook documentation for this module.
 */

/**
 * Allows other modules to alter the transaction error before it's
 * displayed to a user.
 *
 * @param $error string
 *   The error message that's displayed to the user.
 * @param $transaction
 *   The commerce payment transaction.
 * @param $response
 *   The response from Braintree API.
 */
function hook_commerce_braintree_transaction_error_alter(&$error, $transaction, $response) {
  $error = t('The billing information you entered is invalid.');
}
