root = exports ? this
root.cachedData = {}
class Handlers
  # constructor :: ([Question], [Action]) -> Context
  constructor: (initQuestionsList, initActionsList) ->
    @_questions = new root.Questions(initQuestionsList)
    @_actions = new root.Actions(initActionsList)
    @_showQuestionRow = new root.ShowQuestionRow()
    @_showActionRow = new root.ShowActionRow()
    @_setDefaultState()
    @_setHandlers()


  # _setDefaultState () -> undefined
  _setDefaultState: =>
    questionsDOM = @_prepareQuestionsList()
    actionsDOM = @_prepareActionsList()
    @_questionsList = new root.QuestionsList('#questionList .middle', "questionsActionsTable", questionsDOM)
    @_actionsList = new root.ActionsList('#actionList .middle', "questionsActionsTable", actionsDOM)
    @_questionsList.show()
    @_actionsList.show()
    return


  # _prepareQuestionsList :: () -> DocumentFragment
  _prepareQuestionsList: =>
    questionsDOM = @_questions.getAllElementsAsDOM(@_showQuestionRow)
    [].forEach.call(questionsDOM.querySelectorAll("tr"), (element) =>
      viewId = element.data
      element.querySelector("td:first-child")
        .addEventListener("click", =>
          @_questions.showDialog(viewId, true)
        )
    )
    questionsDOM


  # _prepareActionsList :: () -> DocumentFragment
  _prepareActionsList: =>
    actionsDOM = @_actions.getAllElementsAsDOM(@_showActionRow)
    [].forEach.call(actionsDOM.querySelectorAll("tr"), (element) =>
      viewId = element.data
      element.querySelector("td:first-child")
        .addEventListener("click", =>
          @_actions.showDialog(viewId, true)
        )
    )
    actionsDOM


  # _setHandler :: () -> undefined
  _setHandlers: =>
    jQuery("#questionList .addElement").on('click', @_addNewQuestion)
    actionSave = @_newEntryRequest("/createAction/", @_createNewAction)
    actionDialog = new root.ActionDialog('getHTML?name=actionDialog', actionSave)
    jQuery("#actionList .addElement").click( ->
      new root.ActionDialog('getHTML?name=actionDialog', actionSave).show()
    )
    return


  # _addNewQuestion :: () -> undefined
  _addNewQuestion: =>
    jQuery.getJSON('getHTML?name=chooseQuestionTypeDialog', null, (data) =>
      @_showChooseQuestionType(data.html)
    )
    return


  # _showChooseQuestionType :: String -> undefined
  _showChooseQuestionType: (html) =>
    BootstrapDialog.show(
      message: html
      title: 'Wybierz typ pytania'
      closable: false
      buttons: [
        label: 'Anuluj'
        action: (dialog) ->
          dialog.close()
      ]
      onshown: (dialog) =>
        @_setQuestionsHandlers(dialog)
    )
    return


  # _setQuestionsHandlers :: BootstrapDialog -> undefined
  _setQuestionsHandlers: (dialog) =>
    simpleQuestionSave = @_newEntryRequest("/createSimpleQuestion/", @_createNewQuestion)
    simpleQuestionDialog = new root.SimpleQuestionDialog('getHTML?name=simpleQuestionDialog', simpleQuestionSave)
    sortQuestionSave = @_newEntryRequest("/createSortQuestion/", @_createNewQuestion)
    sortQuestionDialog = new root.SortQuestionDialog('getHTML?name=sortQuestionDialog', sortQuestionSave)
    multipleChoiceSave = @_newEntryRequest("/createMultipleChoiceQuestion/", @_createNewQuestion)
    multipleChoiceQuestionDialog = new root.MultipleChoiceQuestionDialog('getHTML?name=multipleChoiceQuestionDialog', multipleChoiceSave)
    jQuery("#dialog button").click( -> dialog.close())
    jQuery("#simpleQuestion").click( -> simpleQuestionDialog.show())
    jQuery("#sortQuestion").click( -> sortQuestionDialog.show())
    jQuery("#multipleChoiceQuestion").click( -> multipleChoiceQuestionDialog.show())
    jQuery("#dialog button").click( -> dialog.close())
    return


  ###
  # type RequestData =
  #   QuestionData
  # | ActionData
  ###
  ###
  # type ResponseData = {
  #   questionsList :: [Question],
  #   success       :: Boolean
  # }
  # | {
  #   actionsList   :: [Action],
  #   success       :: Boolean
  # }
  # | {
  #   success       :: Boolean,
  #   exceptionType :: String,
  #   message       :: String
  # }
  ###
  # _newEntryRequest :: (String, (ResponseData -> undefined))
  #                     -> (RequestData, BootstrapDialog) -> undefined
  _newEntryRequest: (url, callback) =>
    (data, dialog) =>
      jQuery.ajaxSetup(
        headers: { "X-CSRFToken": getCookie("csrftoken") }
      )
      jQuery.ajax(
        type: 'POST'
        dataType: 'json'
        data: (jsonData: JSON.stringify(data))
        url: url
        success: (recvData) =>
          if not recvData.success
            if recvData.exceptionType is 'DuplicateName'
              dialog.showNameDuplicatedError()
            else
              @_displayError(recvData.message)
          else
            callback(recvData)
            dialog.close()
      )
      return


  # _createNewQuestion :: {questionsList :: [Question], success :: Boolean} -> undefined
  _createNewQuestion: (data) =>
    @_questions.setElements(data.questionsList)
    toAdd = @_prepareQuestionsList()
    @_questionsList.replaceElements(toAdd)
    return


  # _createNewAction :: ({actionsList :: [Action], success :: Boolean}) -> undefined
  _createNewAction: (data) =>
    @_actions.setElements(data.actionsList)
    toAdd = @_prepareActionsList()
    @_actionsList.replaceElements(toAdd)
    return


  # _displayError :: String -> undefined
  _displayError: (message) ->
    BootstrapDialog.show(
      message: message
      title: 'Wystąpił błąd'
      type: BootstrapDialog.TYPE_DANGER
    )
    return

jQuery(document).ready( ->
  new Handlers(root.questionsList, root.actionsList)
)
