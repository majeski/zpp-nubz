root = exports ? this
window.onerror = do ->
  timeoutId = 0
  (errorMsg, url, lineNumber) ->
    toSend =
      jsonData:
        JSON.stringify(
          url: url
          lineNumber: lineNumber
          errorMsg: errorMsg
        )
    clearTimeout timeoutId
    timeoutId = setTimeout ( ->
      jQuery.ajax(
        type: 'POST'
        dataType: 'json'
        url: '/errorReporting/'
        data: toSend
        error: () -> window.onerror(errorMsg, url, lineNumber)
      )), 1000
