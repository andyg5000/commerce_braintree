(function($) {
  Drupal.behaviors.commerceBraintreeDropin = {
    attach: function(context, settings) {
      // Init Braintree Drop-in UI so that the form is insterted into the DOM.
      if ($('#commerce-braintree-dropin-container', context).length > 0 && Drupal.settings.commerceBraintreeDropinToken != undefined && braintree != undefined) {
        $('#commerce-braintree-dropin-container', context).once('commerce-braintree-dropin-setup', function() {
          braintree.setup(
          Drupal.settings.commerceBraintreeDropinToken,
          'dropin', {
            container: 'commerce-braintree-dropin-container',
            onError: function(payload) {
              // Show the primary submitButton button and remove the duplicate one
              $('.checkout-continue').each(function() {
                if ($(this).attr('disabled') == true || $(this).attr('disabled') == 'disabled') {
                  $(this).remove();
                }
                $(this).show();
                $('.checkout-processing').hide();
              });
            }
          });
        });
      }

      // Braintree hijacks all submit buttons for this form. Simulate the back
      // button to make sure back submit still works.
      $('.checkout-cancel,.checkout-back', context).click(function(e) {
        e.preventDefault();
        window.history.back();
      });

    }
  }
})(jQuery);
