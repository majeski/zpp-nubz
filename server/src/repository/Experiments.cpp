#include <boost/none.hpp>

#include <utils/fp_algorithm.h>

#include <db/table/Experiments.h>

#include "DefaultRepo.h"
#include "Experiments.h"
#include "error/DuplicateName.h"
#include "error/InvalidData.h"

namespace repository {

using Table = db::table::Experiments;
using Impl = repository::detail::DefaultRepoID<Table>;

namespace {
Table::Sql::in_t toDB(const Experiments::LazyExperiment &experiment, std::int32_t state);
Table::ContentData::Survey surveyToDB(const Experiments::LazyExperiment::Survey &survey);

Experiments::Experiment fromDB(db::DatabaseSession &session, const Table::Sql::out_t &experiment);

Experiments::LazyExperiment lazyFromDB(const Table::Sql::out_t &experiment);
Experiments::LazyExperiment::Survey lazySurveyFromDB(const Table::ContentData::Survey &survey);
}

Experiments::Experiments(db::DatabaseSession &session) : session(session) {
}

Experiments::Experiment Experiments::get(std::int32_t ID) {
    if (auto exp = getOpt(ID)) {
        return exp.value();
    } else {
        throw InvalidData{"there is no experiment with given id"};
    }
}

boost::optional<Experiments::Experiment> Experiments::getOpt(std::int32_t ID) {
    if (auto dbExperiment = Impl::get(session, ID)) {
        return fromDB(session, dbExperiment.value());
    } else {
        return {};
    }
}

boost::optional<Experiments::LazyExperiment> Experiments::getLazy(std::int32_t ID) {
    if (auto dbExperiment = Impl::get(session, ID)) {
        return lazyFromDB(dbExperiment.value());
    } else {
        return {};
    }
}

void Experiments::start(std::int32_t ID) {
    checkID(ID);
    if (getActive()) {
        throw InvalidData{"you can only have one active experiment"};
    }
    if (getState(ID) != State::Ready) {
        throw InvalidData{"you can only start ready experiments"};
    }

    auto sql = Table::Sql::update()
                   .where(Table::ID == ID)
                   .set(Table::State, State::Active)
                   .set(Table::StartDate, boost::gregorian::day_clock::local_day());
    session.execute(sql);
}

void Experiments::finishActive() {
    if (!getActive()) {
        throw InvalidData{"there is no active experiment"};
    }
    auto sql = Table::Sql::update()
                   .where(Table::State == State::Active)
                   .set(Table::State, State::Finished)
                   .set(Table::FinishDate, boost::gregorian::day_clock::local_day());
    session.execute(sql);
}

boost::optional<Experiments::Experiment> Experiments::getActive() {
    auto sql = Table::Sql::select().where(Table::State == State::Active);
    if (auto dbTuple = session.getResult(sql)) {
        return fromDB(session, dbTuple.value());
    } else {
        return {};
    }
}

boost::optional<Experiments::LazyExperiment> Experiments::getLazyActive() {
    auto sql = Table::Sql::select().where(Table::State == State::Active);
    if (auto dbTuple = session.getResult(sql)) {
        return lazyFromDB(dbTuple.value());
    } else {
        return {};
    }
}

std::vector<Experiments::LazyExperiment> Experiments::getAllReady() {
    return getAllWithState(State::Ready);
}

std::vector<Experiments::LazyExperiment> Experiments::getAllFinished() {
    return getAllWithState(State::Finished);
}

std::vector<Experiments::LazyExperiment> Experiments::getAllWithState(State state) {
    auto sql = Table::Sql::select().where(Table::State == state);

    auto result = std::vector<LazyExperiment>{};
    utils::transform(session.getResults(sql), result, lazyFromDB);
    return result;
}

void Experiments::clone(std::int32_t ID, const std::string &name) {
    cloneCheck(ID, name);
    cloneExec(ID, name);
}

void Experiments::cloneCheck(std::int32_t ID, const std::string &name) {
    checkID(ID);
    checkName(name);
}

void Experiments::cloneExec(std::int32_t ID, const std::string &name) {
    auto original = getLazy(ID).value();
    original.name = name;
    insertExec(&original);
}

void Experiments::insert(Experiments::LazyExperiment *experiment) {
    insertCheck(*experiment);
    insertExec(experiment);
}

void Experiments::insertCheck(const LazyExperiment &experiment) {
    checkName(experiment.name);
    checkExperiment(experiment);
}

void Experiments::insertExec(LazyExperiment *experiment) {
    experiment->startDate = boost::none;
    experiment->finishDate = boost::none;
    retainData(*experiment);
    experiment->ID = Impl::insert(session, toDB(*experiment, State::Ready));
}

void Experiments::update(const Experiments::LazyExperiment &experiment) {
    updateCheck(experiment);
    updateExec(experiment);
}

void Experiments::updateCheck(const LazyExperiment &experiment) {
    checkID(experiment.ID);
    if (getState(experiment.ID) != State::Ready) {
        throw InvalidData{"you can only update ready experiments"};
    }
    // name updated? need to check for duplicates
    if (get(experiment.ID).name != experiment.name) {
        checkName(experiment.name);
    }
    checkExperiment(experiment);
}

void Experiments::updateExec(const LazyExperiment &experiment) {
    releaseData(getLazy(experiment.ID).value());
    retainData(experiment);

    auto dbContent = std::get<Table::FieldContent>(toDB(experiment, State::Ready)).value;
    auto sql = Table::Sql::update()
                   .where(Table::ID == experiment.ID)
                   .set(Table::Name, experiment.name)
                   .set(Table::Content, dbContent);
    session.execute(sql);
}

void Experiments::remove(std::int32_t ID) {
    checkID(ID);
    if (getState(ID) == Experiments::State::Active) {
        throw InvalidData{"cannot remove active experiment"};
    }
    releaseData(getLazy(ID).value());
    Impl::remove(session, ID);
}

Experiments::State Experiments::getState(std::int32_t ID) {
    auto sql = db::sql::Select<Table::FieldID, Table::FieldState>{}.where(Table::ID == ID);
    auto stateNum = std::get<Table::FieldState>(session.getResult(sql).value()).value;
    return static_cast<State>(stateNum);
}

void Experiments::retainData(const repository::Experiments::LazyExperiment &experiment) {
    retainActions(experiment);
    retainQuestions(experiment.surveyBefore);
    retainQuestions(experiment.surveyAfter);
}

void Experiments::releaseData(const repository::Experiments::LazyExperiment &experiment) {
    releaseActions(experiment);
    releaseQuestions(experiment.surveyBefore);
    releaseQuestions(experiment.surveyAfter);
}

void Experiments::retainActions(const Experiments::LazyExperiment &experiment) {
    auto repo = Actions{session};
    for (const auto actions : {experiment.actions, experiment.breakActions}) {
        for (auto action : actions) {
            repo.incReferenceCount(action);
        }
    }
}

void Experiments::releaseActions(const Experiments::LazyExperiment &experiment) {
    auto repo = Actions{session};
    for (const auto actions : {experiment.actions, experiment.breakActions}) {
        for (auto action : actions) {
            repo.decReferenceCount(action);
        }
    }
}

void Experiments::retainQuestions(const LazyExperiment::Survey &survey) {
    {
        auto repo = SimpleQuestions{session};
        for (auto q : survey.simpleQuestions) {
            repo.incReferenceCount(q);
        }
    }
    {
        auto repo = MultipleChoiceQuestions{session};
        for (auto q : survey.multipleChoiceQuestions) {
            repo.incReferenceCount(q);
        }
    }
    {
        auto repo = SortQuestions{session};
        for (auto q : survey.sortQuestions) {
            repo.incReferenceCount(q);
        }
    }
}

void Experiments::releaseQuestions(const LazyExperiment::Survey &survey) {
    {
        auto repo = SimpleQuestions{session};
        for (auto q : survey.simpleQuestions) {
            repo.decReferenceCount(q);
        }
    }
    {
        auto repo = MultipleChoiceQuestions{session};
        for (auto q : survey.multipleChoiceQuestions) {
            repo.decReferenceCount(q);
        }
    }
    {
        auto repo = SortQuestions{session};
        for (auto q : survey.sortQuestions) {
            repo.decReferenceCount(q);
        }
    }
}

void Experiments::checkID(std::int32_t ID) {
    using namespace db::sql;
    auto sql = Select<Table::FieldID>{}.where(Table::ID == ID);
    if (!session.getResult(sql)) {
        throw InvalidData{"there is no experiment with given id"};
    }
}

void Experiments::checkName(const std::string &name) {
    if (name.empty()) {
        throw InvalidData{"experiment name cannot be empty"};
    }

    auto sql = db::sql::Select<Table::FieldName>{}.where(Table::Name == name);
    if (session.getResult(sql)) {
        throw DuplicateName{"experiment with given name already exists"};
    }
}

void Experiments::checkExperiment(const LazyExperiment &experiment) {
    checkSurvey(experiment.surveyBefore);
    checkSurvey(experiment.surveyAfter);
}

void Experiments::checkSurvey(const LazyExperiment::Survey &survey) {
    using QuestionType = Experiment::Survey::QuestionType;

    if (utils::count(survey.typesOrder, QuestionType::Simple) != survey.simpleQuestions.size() ||
        utils::count(survey.typesOrder, QuestionType::Sort) != survey.sortQuestions.size() ||
        utils::count(survey.typesOrder, QuestionType::MultipleChoice) !=
            survey.multipleChoiceQuestions.size()) {
        throw InvalidData("survey's types order doesn't match questions");
    }
}

namespace {
Table::Sql::in_t toDB(const Experiments::LazyExperiment &experiment, std::int32_t state) {
    auto content = Table::ContentData{};
    content.actions = experiment.actions;
    content.breakActions = experiment.breakActions;
    content.surveyBefore = surveyToDB(experiment.surveyBefore);
    content.surveyAfter = surveyToDB(experiment.surveyAfter);

    return std::make_tuple(Table::FieldName{experiment.name},
                           Table::FieldState{state},
                           Table::FieldStartDate{},
                           Table::FieldFinishDate{},
                           Table::FieldContent{content});
}

Table::ContentData::Survey surveyToDB(const Experiments::LazyExperiment::Survey &survey) {
    auto dbSurvey = Table::ContentData::Survey{};
    dbSurvey.typesOrder = {survey.typesOrder.begin(), survey.typesOrder.end()};
    dbSurvey.simpleQuestions = survey.simpleQuestions;
    dbSurvey.multipleChoiceQuestions = survey.multipleChoiceQuestions;
    dbSurvey.sortQuestions = survey.sortQuestions;
    return dbSurvey;
}

Experiments::Experiment fromDB(db::DatabaseSession &session, const Table::Sql::out_t &experiment) {
    auto lazy = lazyFromDB(experiment);
    auto res = Experiment{};
    res.ID = lazy.ID;
    res.name = lazy.name;
    res.startDate = lazy.startDate;
    res.finishDate = lazy.finishDate;

    // actions
    {
        auto actionsRepo = Actions{session};
        auto getAction = [&](auto actionId) { return actionsRepo.get(actionId); };
        utils::transform(lazy.actions, res.actions, getAction);
        utils::transform(lazy.breakActions, res.breakActions, getAction);
    }

    // surveys
    {
        auto simpleQRepo = SimpleQuestions{session};
        auto sortQRepo = SortQuestions{session};
        auto multiChoiceQRepo = MultipleChoiceQuestions{session};

        auto getSurvey = [&](auto &survey) {
            auto res = Experiment::Survey{};
            res.typesOrder = survey.typesOrder;
            utils::transform(survey.simpleQuestions, res.simpleQuestions, [&](auto qId) {
                return simpleQRepo.get(qId);
            });
            utils::transform(survey.sortQuestions, res.sortQuestions, [&](auto qId) {
                return sortQRepo.get(qId);
            });
            utils::transform(survey.multipleChoiceQuestions,
                             res.multipleChoiceQuestions,
                             [&](auto qId) { return multiChoiceQRepo.get(qId); });
            return res;
        };

        res.surveyBefore = getSurvey(lazy.surveyBefore);
        res.surveyAfter = getSurvey(lazy.surveyAfter);
    }

    return res;
}

Experiments::LazyExperiment lazyFromDB(const Table::Sql::out_t &experiment) {
    auto res = Experiments::LazyExperiment{};
    res.ID = std::get<Table::FieldID>(experiment).value;
    res.name = std::get<Table::FieldName>(experiment).value;
    res.startDate = std::get<Table::FieldStartDate>(experiment).value;
    res.finishDate = std::get<Table::FieldFinishDate>(experiment).value;

    auto content = std::get<Table::FieldContent>(experiment).value;
    res.actions = content.actions;
    res.breakActions = content.breakActions;
    res.surveyBefore = lazySurveyFromDB(content.surveyBefore);
    res.surveyAfter = lazySurveyFromDB(content.surveyAfter);
    return res;
}

Experiments::LazyExperiment::Survey lazySurveyFromDB(const Table::ContentData::Survey &survey) {
    auto res = Experiments::LazyExperiment::Survey{};
    utils::transform(survey.typesOrder, res.typesOrder, [](auto type) {
        return static_cast<Experiments::LazyExperiment::QuestionType>(type);
    });
    res.simpleQuestions = survey.simpleQuestions;
    res.multipleChoiceQuestions = survey.multipleChoiceQuestions;
    res.sortQuestions = survey.sortQuestions;
    return res;
}
}
}