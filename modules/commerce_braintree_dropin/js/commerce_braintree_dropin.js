/**
 * @file
 * Defines behaviors for the Braintree Drop-in UI payment method form.
 */

(function($) {
    Drupal.behaviors.commerceBraintreeDropin = {
        attach: function (context, settings) {
            if (typeof settings.commerceBraintreeDropin !== 'undefined') {
                var $dropInForm = $(context).find('#commerce-braintree-dropin-container').once();
                if ($dropInForm.length) {
                    var waitForSdk = setInterval(function () {
                        if (typeof braintree !== 'undefined' && typeof braintree.dropin !== 'undefined') {
                            clearInterval(waitForSdk);
                            var $form = $dropInForm.closest('form');
                            Drupal.braintreeDropIn = new Drupal.commerceBraintreeDropin($form, settings.commerceBraintreeDropin);
                            Drupal.braintreeDropIn.bootstrap();
                        }
                    }, 100);
                }
            }
        }
    };

    Drupal.commerceBraintreeDropin = function ($form, settings) {
        this.settings = settings;
        this.$form = $form;
        this.fromId = this.$form.attr('id');
        this.$submit = this.$form.find('[name=op]');
        this.error = '';
        this.$nonceInput = $('input[name="' + settings.nonceInput +'"]', $form);
        return this;
    };

    Drupal.commerceBraintreeDropin.prototype.bootstrap = function () {
        var options = this.getOptions();
        var button = document.querySelector(Drupal.braintreeDropIn.settings.submitSelector);
        braintree.dropin.create({
            authorization: options.clientToken,
            container: '#' + options.containerId
        }, function (createErr, instance) {
            button.addEventListener('click', function (e) {
                e.preventDefault();
                instance.requestPaymentMethod(function (requestPaymentMethodErr, payload) {
                    if (requestPaymentMethodErr) {
                        // No payment method is available.
                        // An appropriate error will be shown in the UI.
                        console.error(requestPaymentMethodErr);
                        Drupal.braintreeDropIn.resetSubmitBtn();
                        return;
                    }

                    Drupal.braintreeDropIn.payloadReceived(payload);
                });
            });
        });
    };

    Drupal.commerceBraintreeDropin.prototype.resetSubmitBtn = function () {
        $('.checkout-processing', this.$form).addClass('element-invisible');
        this.$submit.removeAttr('disabled');
        this.$submit.next('.checkout-continue').remove();
        this.$submit.show();
    };

    Drupal.commerceBraintreeDropin.prototype.getOptions = function () {
        var options = {
            onReady: $.proxy(this.onReady, this),
            onError: $.proxy(this.onError, this),
            id: this.fromId,
            container: '#commerce-braintree-dropin-container',
            authorization: this.settings.clientToken,
            paypal: {
                flow: 'vault'
            }
        };

        options = $.extend(options, this.settings);
        return options;
    };

    Drupal.commerceBraintreeDropin.prototype.payloadReceived = function (payload) {
        this.$submit.attr({'disabled' : 'disabled'});
        this.$nonceInput.val(payload.nonce);
        this.$form.submit();
    };

    // Global event callback.
    Drupal.commerceBraintreeDropin.prototype.onError = function (response) {
       this.errorMsg(response)
    };
})(jQuery);
