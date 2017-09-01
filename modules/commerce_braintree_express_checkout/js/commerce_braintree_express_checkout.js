(function($) {

    Drupal.behaviors.commerceBraintreeExpressCheckout = {
        attach: function (context, settings) {
            if (typeof settings.commerceBraintreeExpressCheckout !== 'undefined') {
                var $expressCheckoutForm = $(context).find('#' + settings.commerceBraintreeExpressCheckout.buttonId).once();
                if ($expressCheckoutForm.length) {
                    var waitForSdk = setInterval(function () {
                        if (typeof braintree !== 'undefined' && typeof paypal !== 'undefined') {
                            clearInterval(waitForSdk);
                            var $form = $expressCheckoutForm.closest('form');
                            Drupal.braintreeExpressCheckout = new Drupal.commerceBraintreeExpressCheckout($form, settings.commerceBraintreeExpressCheckout);
                            Drupal.braintreeExpressCheckout.bootstrap();

                            // Handle toggling of the checkout complete button.
                            Drupal.braintreeExpressCheckout.paymentMethodEvents();
                        }
                    }, 100);
                }

            }
        }
    }

    Drupal.commerceBraintreeExpressCheckout = function ($form, settings) {
        this.settings = settings;
        this.$form = $form;
        this.fromId = this.$form.attr('id');
        this.$submit = this.$form.find('[name=op]');
        this.error = '';
        this.ecButtonSelector = '#' + settings.buttonId;
        this.$ecButton = this.$form.find(this.ecButtonSelector);
        this.$payloadInput = $('input[name="' + settings.payloadInput +'"]', $form);
        this.$nonceInput = $('input[name="' + settings.nonceInput +'"]', $form);
        return this;
    };

    Drupal.commerceBraintreeExpressCheckout.prototype.bootstrap = function () {
        braintree.client.create(this.getOptions(), function(clientErr, clientInstance){
            Drupal.braintreeExpressCheckout.getButton(clientErr, clientInstance);
        });
    };

    Drupal.commerceBraintreeExpressCheckout.prototype.getOptions = function () {
        var options = {
            authorization: this.settings.clientToken
        };

        options = $.extend(options, this.settings);

        return options;
    };

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

                // Payment handler.
                payment: function () {
                    return paypalCheckoutInstance.createPayment({
                        flow: Drupal.braintreeExpressCheckout.settings.flow,
                        amount: Drupal.braintreeExpressCheckout.settings.amount,
                        currency: Drupal.braintreeExpressCheckout.settings.currencyCode,
                        enableShippingAddress: true
                    });
                },

                // Authorization handler.
                onAuthorize: function (data, actions) {
                    return paypalCheckoutInstance.tokenizePayment(data)
                        .then(function (payload) {
                            // Populate the payload input if available.
                            if (Drupal.braintreeExpressCheckout.$payloadInput.length > 0) {
                                Drupal.braintreeExpressCheckout.$payloadInput.val(JSON.stringify(payload));
                            }

                            // Populate the nonce input if available.
                            var $nonceField = $('input[name="' + Drupal.braintreeExpressCheckout.settings.nonceInput + '"]');
                            if ($nonceField.length > 0) {
                               $nonceField.val(payload.nonce);
                            }

                            // Submit the form.
                            Drupal.braintreeExpressCheckout.$submit.trigger('click');
                        });
                },

                // Cancel handler.
                onCancel: function (data) {
                    console.log("checkout.js payment cancelled", JSON.stringify(data, 0, 2));
                },

                // Error handler.
                onError: function (err) {
                    console.error("checkout.js error", err);
                }
            }, Drupal.braintreeExpressCheckout.ecButtonSelector).then(function () {
                // Button has been initialized.
            });
        });
    };

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

    Drupal.commerceBraintreeExpressCheckout.prototype.toggleCheckout = function(element) {
        // Always show submit and hide PayPal Express checkout when the nonce
        // field is available and filled.
        if (Drupal.braintreeExpressCheckout.$nonceInput.length > 0 && Drupal.braintreeExpressCheckout.$nonceInput.val() != '') {
            Drupal.braintreeExpressCheckout.$submit.show();
            Drupal.braintreeExpressCheckout.$ecButton.hide();
            return;
        }

        // If Express Checkout method is selected, hide the Commerce submit
        // button and show the Express Checkout payment button.
        if (element.val() == Drupal.braintreeExpressCheckout.settings.instanceId) {
            Drupal.braintreeExpressCheckout.$submit.hide();
            Drupal.braintreeExpressCheckout.$ecButton.show();
        }
        else {
            Drupal.braintreeExpressCheckout.$submit.show();
            Drupal.braintreeExpressCheckout.$ecButton.hide();
        }
    };
})(jQuery);
