(function($) {
  Drupal.behaviors.commerceBraintreeDropin = {
    attach: function(context, settings) {
      // Init Braintree Drop-in UI so that the form is insterted into the DOM.
      if ($('#commerce-braintree-dropin-container', context).length > 0 && Drupal.settings.commerceBraintreeDropinToken != undefined && braintree != undefined) {
        $('#commerce-braintree-dropin-container', context).once('braintreeSetup', function() {
          braintree.setup(
          Drupal.settings.commerceBraintreeDropinToken,
          'dropin', {
            container: 'commerce-braintree-dropin-container'
          });
        });
      }

      // Form validation and the Braintree JS api prevent submitting the form
      // multiple times. We need to remove the "undo" the event handler that
      // commerce_checkout.js has added to the submit button.
      var submitButton = $('.checkout-continue', context);
      submitButton.click(function() {
        setTimeout(function() {
          // Show the primary submitButton button and hide the duplicate one.
          submitButton.show();
          $('.checkout-continue').each(function() {
            if ($(this).attr('disabled') == true || $(this).attr('disabled') == 'disabled') {
              $(this).remove();
            }
          })
        }, 1);
      });

      // Braintree hijacks all submit buttons for this form. Simulate the back
      // button to make sure back submit still works.
      $('.checkout-cancel,.checkout-back', context).click(function(e) {
        e.preventDefault();
        window.history.back();
      });

    }
  }
})(jQuery);
