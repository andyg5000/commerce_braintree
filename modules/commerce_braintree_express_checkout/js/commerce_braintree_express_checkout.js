(function($) {

    Drupal.behaviors.commerceBraintreeExpressCheckout = {
        attach: function (context, settings) {
            if (typeof settings.commerceBraintreeExpressCheckout !== 'undefined') {
                var $expressCheckoutForm = $(context).find('#' + settings.commerceBraintreeExpressCheckout.buttonId).once();
                if ($expressCheckoutForm.length) {
                    var waitForSdk = setInterval(function () {

                        // Make sure that braintree and paypal SDKs have been loaded.
                        if (typeof braintree !== 'undefined' && typeof paypal !== 'undefined') {
                            clearInterval(waitForSdk);
                            var $form = $expressCheckoutForm.closest('form');

                            // Set up the Express Checkout button and event handlers.
                            Drupal.braintreeExpressCheckout = new Drupal.commerceBraintreeExpressCheckout($form, settings.commerceBraintreeExpressCheckout);
                            Drupal.braintreeExpressCheckout.bootstrap();

                            // Handle toggling of the checkout complete button.
                            Drupal.braintreeExpressCheckout.paymentMethodEvents();
                        }
                    }, 100);
                }

            }
        }
    };

    /**
     * Initializes the object for Express Checkout.
     *
     * @param $form
     *   The jQuery DOM element where the form is scoped.
     * @param settings
     *   The Express Checkout settings provided by Drupal.
     *
     * @returns {Drupal}
     */
    Drupal.commerceBraintreeExpressCheckout = function ($form, settings) {
        this.settings = settings;
        this.$form = $form;
        this.fromId = this.$form.attr('id');
        this.$submit = this.$form.find(settings.submitSelector);
        this.ecButtonSelector = '#' + settings.buttonId;
        this.$ecButton = this.$form.find(this.ecButtonSelector);
        this.$payloadInput = $('input[name="' + settings.payloadInput +'"]', $form);
        this.$nonceInput = $(settings.nonceSelector);
        return this;
    };

    /**
     * Creates the Braintree client and PayPal Express Checkout button.
     */
    Drupal.commerceBraintreeExpressCheckout.prototype.bootstrap = function () {
        braintree.client.create(this.settings.options, function(clientErr, clientInstance){
            Drupal.braintreeExpressCheckout.getButton(clientErr, clientInstance);
        });
    };

    /**
     * Sets up the Express Checkout button and events.
     * @param clientErr
     * @param clientInstance
     */
    Drupal.commerceBraintreeExpressCheckout.prototype.getButton = function(clientErr, clientInstance) {

        if (clientErr) {
            console.error("Error creating client:", clientErr);
            return;
        }

        // Create a PayPal Checkout workflow.
        braintree.paypalCheckout.create({
            client: clientInstance
        }, function (paypalCheckoutErr, paypalCheckoutInstance) {
            if (paypalCheckoutErr) {
                console.error("Error creating PayPal Checkout:", paypalCheckoutErr);
                return;
            }

            // Sets up the express checkout button properties and events.
            paypal.Button.render({
                // Environment of sandbox or production.
                env: Drupal.braintreeExpressCheckout.settings.environment,
                commit: true,

                // Button styles.
                style: Drupal.braintreeExpressCheckout.settings.buttonStyle,

                // Creates the Braintree Payment info for Express Checkout.
                payment: function () {
                    return paypalCheckoutInstance.createPayment( Drupal.braintreeExpressCheckout.settings.createPaymentOptions);
                },

                // Authorization handler fires when Express Checkout is
                // successful.
                onAuthorize: function (data, actions) {
                    return paypalCheckoutInstance.tokenizePayment(data)
                        .then(function (payload) {
                            Drupal.braintreeExpressCheckout.payloadReceived(payload);
                        });
                },

                // Cancel handler.
                onCancel: function (data) {},

                // Error handler.
                onError: function (err) {}

            }, Drupal.braintreeExpressCheckout.ecButtonSelector).then(function () {
                // Button has been initialized.
            });
        });
    };

    /**
     * Adds event handlers to the payment method selector form on checkout.
     */
    Drupal.commerceBraintreeExpressCheckout.prototype.paymentMethodEvents = function() {
        var paymentSelector = $('input[name="commerce_payment[payment_method]"',  Drupal.braintreeExpressCheckout.$form);
        if (paymentSelector.length > 0) {
            // Add event handlers to the payment selector to hide
            // the default checkout button when EC is selected and
            // show the PayPal EC button.
            paymentSelector.change(function() {
                Drupal.braintreeExpressCheckout.toggleCheckout($(this));
            });

            // Set initial toggle on load.
            Drupal.braintreeExpressCheckout.toggleCheckout(paymentSelector.filter(':checked'));
        }
    };

    /**
     * Handles the payload response from PayPal.
     *
     * @param payload
     *   The Express Checkout payload.
     */
    Drupal.commerceBraintreeExpressCheckout.prototype.payloadReceived = function (payload) {
        // Populate the payload input if available.
        if (this.$payloadInput.length > 0) {
            this.$payloadInput.val(JSON.stringify(payload));
        }

        // Populate the nonce input if available.
        if (this.$nonceInput.length > 0) {
            this.$nonceInput.val(payload.nonce);
        }

        // Submit the form and disable the submit button.
        this.$submit.trigger('click');
        this.$submit.attr({'disabled' : 'disabled'});
    };

    /**
     * Handles toggling the checkout submit and Express Checkout buttons.
     *
     * @param element
     *   A jQuery DOM element.
     */
    Drupal.commerceBraintreeExpressCheckout.prototype.toggleCheckout = function(element) {
        // Always show submit and hide PayPal Express checkout when the nonce
        // field is available and filled.
        if (this.$nonceInput.length > 0 && this.$nonceInput.val() != '') {
            this.$submit.show();
            this.$ecButton.hide();
            return;
        }

        // If Express Checkout method is selected, hide the Commerce submit
        // button and show the Express Checkout payment button.
        if (element.val() == this.settings.instanceId) {
            this.$submit.hide();
            this.$ecButton.show();
        }
        else {
            this.$submit.show();
            this.$ecButton.hide();
        }
    };
})(jQuery);
