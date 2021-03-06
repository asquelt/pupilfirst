[%bs.raw {|require("./CommunitiesShow__Root.css")|}];

open CommunitiesShow__Types;

let str = React.string;

let t = I18n.t(~scope="components.CommunitiesShow__Root");

type solution = [ | `Solved | `Unsolved | `Unselected];

type sortCriterion = [ | `CreatedAt | `LastActivityAt | `Views];

type sortDirection = [ | `Ascending | `Descending];

type loading =
  | NotLoading
  | Reloading
  | LoadingMore;

type filter = {
  topicCategory: option(TopicCategory.t),
  solution,
  title: option(string),
  target: option(Target.t),
};

type state = {
  loading,
  topics: Topics.t,
  filterString: string,
  filter,
  totalTopicsCount: int,
  sortCriterion,
  sortDirection,
};

type action =
  | SelectTopicCategory(TopicCategory.t)
  | DeselectTopicCategory
  | SetSearchString(string)
  | UnsetSearchString
  | UpdateFilterString(string)
  | LoadTopics(option(string), bool, array(Topic.t), int)
  | BeginLoadingMore
  | BeginReloading
  | SetSolutionFilter(solution)
  | UpdateSortDirection(sortDirection)
  | UpdateSortCriterion(sortCriterion);

let reducer = (state, action) => {
  switch (action) {
  | SelectTopicCategory(topicCategory) => {
      ...state,
      filter: {
        ...state.filter,
        topicCategory: Some(topicCategory),
      },
      filterString: "",
    }
  | DeselectTopicCategory => {
      ...state,
      filter: {
        ...state.filter,
        topicCategory: None,
      },
    }
  | SetSearchString(string) => {
      ...state,
      filter: {
        ...state.filter,
        title: Some(string),
      },
      filterString: "",
    }
  | UnsetSearchString => {
      ...state,
      filter: {
        ...state.filter,
        title: None,
      },
    }
  | SetSolutionFilter(solution) => {
      ...state,
      filter: {
        ...state.filter,
        solution,
      },
      filterString: "",
    }
  | UpdateFilterString(filterString) => {...state, filterString}
  | UpdateSortCriterion(sortCriterion) => {...state, sortCriterion}
  | UpdateSortDirection(sortDirection) => {...state, sortDirection}
  | LoadTopics(endCursor, hasNextPage, newTopics, totalTopicsCount) =>
    let updatedTopics =
      switch (state.loading) {
      | LoadingMore =>
        newTopics |> Array.append(state.topics |> Topics.toArray)
      | Reloading => newTopics
      | NotLoading => newTopics
      };

    {
      ...state,
      topics:
        switch (hasNextPage, endCursor) {
        | (_, None)
        | (false, Some(_)) => FullyLoaded(updatedTopics)
        | (true, Some(cursor)) => PartiallyLoaded(updatedTopics, cursor)
        },
      loading: NotLoading,
      totalTopicsCount,
    };
  | BeginLoadingMore => {...state, loading: LoadingMore}
  | BeginReloading => {...state, loading: Reloading}
  };
};

module TopicsQuery = [%graphql
  {|
    query TopicsFromCommunitiesShowRootQuery($communityId: ID!, $topicCategoryId: ID,$targetId: ID, $resolution: TopicResolutionFilter!, $search: String, $after: String, $sortCriterion: TopicSortCriterion!, $sortDirection: SortDirection!) {
      topics(communityId: $communityId, topicCategoryId: $topicCategoryId,targetId: $targetId, search: $search, resolution: $resolution, sortDirection: $sortDirection, sortCriterion: $sortCriterion, first: 10, after: $after) {
        nodes {
          id
          lastActivityAt
          createdAt
          creator {
            name
          }
          likesCount
          liveRepliesCount
          views
          title
          topicCategoryId
          participantsCount
          participants {
            id
            name
            avatarUrl
          }
        }
        pageInfo{
          endCursor,hasNextPage
        }
        totalCount
      }
    }
  |}
];

let getTopics =
    (send, communityId, cursor, filter, sortCriterion, sortDirection) => {
  let topicCategoryId =
    filter.topicCategory |> OptionUtils.map(TopicCategory.id);

  let targetId = filter.target |> OptionUtils.map(Target.id);

  TopicsQuery.make(
    ~communityId,
    ~after=?cursor,
    ~topicCategoryId?,
    ~targetId?,
    ~search=?filter.title,
    ~resolution=filter.solution,
    ~sortCriterion,
    ~sortDirection,
    (),
  )
  |> GraphqlQuery.sendQuery
  |> Js.Promise.then_(response => {
       let newTopics =
         response##topics##nodes
         |> Js.Array.map(topicData => Topic.makeFromJS(topicData));

       send(
         LoadTopics(
           response##topics##pageInfo##endCursor,
           response##topics##pageInfo##hasNextPage,
           newTopics,
           response##topics##totalCount,
         ),
       );

       Js.Promise.resolve();
     })
  |> ignore;
};

let reloadTopics = (communityId, state, send) => {
  send(BeginReloading);
  getTopics(
    send,
    communityId,
    None,
    state.filter,
    state.sortCriterion,
    state.sortDirection,
  );
};

let computeInitialState = target => {
  loading: NotLoading,
  topics: Unloaded,
  filterString: "",
  filter: {
    title: None,
    topicCategory: None,
    target,
    solution: `Unselected,
  },
  totalTopicsCount: 0,
  sortDirection: `Descending,
  sortCriterion: `CreatedAt,
};

let topicsList = (topicCategories, topics) => {
  topics |> ArrayUtils.isEmpty
    ? <div
        className="flex flex-col mx-auto bg-white rounded-md border p-6 justify-center items-center">
        <FaIcon classes="fas fa-comments text-5xl text-gray-400" />
        <h4 className="mt-3 text-base md:text-lg text-center font-semibold">
          {t("empty_topics")->str}
        </h4>
      </div>
    : topics
      |> Js.Array.map(topic =>
           <a
             className="block"
             key={Topic.id(topic)}
             href={"/topics/" ++ Topic.id(topic)}
             ariaLabel={"Topic " ++ Topic.id(topic)}>
             <div
               className="flex items-center border border-transparent hover:bg-gray-100 hover:text-primary-500  hover:border-primary-400">
               <div className="flex-1">
                 <div
                   className="cursor-pointer no-underline flex flex-col p-4 md:px-6 md:py-5">
                   <span className="block">
                     <h4
                       className="community-topic__title text-base font-semibold inline-block break-words leading-snug">
                       {Topic.title(topic) |> str}
                     </h4>
                     <span className="block text-xs text-gray-800 pt-1">
                       <span> {t("topic_posted_by_text")->str} </span>
                       <span className="font-semibold mr-2">
                         {(
                            switch (Topic.creatorName(topic)) {
                            | Some(name) => " " ++ name ++ " "
                            | None => "Unknown "
                            }
                          )
                          |> str}
                       </span>
                       <span className="hidden md:inline-block md:mr-2">
                         {"on "
                          ++ Topic.createdAt(topic)
                             ->DateFns.formatPreset(~year=true, ())
                          |> str}
                       </span>
                       <span
                         className="inline-block md:mt-0 md:px-2 md:border-l border-gray-400">
                         {switch (Topic.lastActivityAt(topic)) {
                          | Some(date) =>
                            <span>
                              <span className="hidden md:inline-block">
                                {t("topic_last_updated_text")->str}
                              </span>
                              {" "
                               ++ DateFns.formatDistanceToNowStrict(
                                    date,
                                    ~addSuffix=true,
                                    (),
                                  )
                               |> str}
                            </span>
                          | None => React.null
                          }}
                       </span>
                     </span>
                   </span>
                   <span className="flex flex-row flex-wrap mt-2">
                     <span
                       className="flex text-center items-center mr-2 py-1 px-2 rounded bg-gray-200"
                       ariaLabel="Likes">
                       <i
                         className="far fa-thumbs-up text-sm text-gray-600 mr-1"
                       />
                       <p className="text-xs font-semibold">
                         {Topic.likesCount(topic) |> string_of_int |> str}
                         <span className="ml-1 hidden md:inline">
                           {Inflector.pluralize(
                              t("topic_stats_likes"),
                              ~count=Topic.likesCount(topic),
                              ~inclusive=false,
                              (),
                            )
                            |> str}
                         </span>
                       </p>
                     </span>
                     <span
                       className="flex justify-between text-center items-center mr-2 py-1 px-2 rounded bg-gray-200"
                       ariaLabel="Replies">
                       <i
                         className="far fa-comment-dots text-sm text-gray-600 mr-1"
                       />
                       <p className="text-xs font-semibold">
                         {Topic.liveRepliesCount(topic)
                          |> string_of_int
                          |> str}
                         <span className="ml-1 hidden md:inline">
                           {Inflector.pluralize(
                              t("topic_stats_replies"),
                              ~count=Topic.liveRepliesCount(topic),
                              ~inclusive=false,
                              (),
                            )
                            |> str}
                         </span>
                       </p>
                     </span>
                     <span
                       className="flex justify-between text-center items-center mr-2 py-1 px-2 rounded bg-gray-200"
                       ariaLabel="Views">
                       <i className="far fa-eye text-sm text-gray-600 mr-1" />
                       <p className="text-xs font-semibold">
                         {Topic.views(topic) |> string_of_int |> str}
                         <span className="ml-1 hidden md:inline">
                           {Inflector.pluralize(
                              t("topic_stats_views"),
                              ~count=Topic.views(topic),
                              ~inclusive=false,
                              (),
                            )
                            |> str}
                         </span>
                       </p>
                     </span>
                     {switch (Topic.topicCategoryId(topic)) {
                      | Some(id) =>
                        let topicCategory =
                          topicCategories
                          |> ArrayUtils.unsafeFind(
                               c => TopicCategory.id(c) == id,
                               "Unable to find topic category with ID: " ++ id,
                             );
                        let (color, _) =
                          StringUtils.toColor(
                            TopicCategory.name(topicCategory),
                          );
                        let style =
                          ReactDOMRe.Style.make(~backgroundColor=color, ());
                        <span
                          className="flex items-center text-xs font-semibold py-1">
                          <div className="w-3 h-3 rounded" style />
                          <span className="ml-1">
                            {TopicCategory.name(topicCategory)->str}
                          </span>
                        </span>;
                      | None => React.null
                      }}
                   </span>
                 </div>
               </div>
               <div className="md:w-1/5">
                 <CommunitiesShow__Participants
                   title=React.null
                   className="flex flex-shrink-0 items-center justify-end pr-4 md:pr-6"
                   participants={Topic.participants(topic)}
                   participantsCount={Topic.participantsCount(topic)}
                 />
               </div>
             </div>
           </a>
         )
      |> React.array;
};

let topicsLoadedData = (totalTopicsCount, loadedTopicsCount) => {
  <div
    className="inline-block mt-2 mx-auto bg-gray-200 text-gray-800 text-xs p-2 text-center rounded font-semibold">
    {(
       totalTopicsCount == loadedTopicsCount
         ? t(
             ~variables=[|
               ("total_topics", string_of_int(totalTopicsCount)),
             |],
             "topics_fully_loaded_text",
           )
         : t(
             ~variables=[|
               ("total_topics", string_of_int(totalTopicsCount)),
               ("loaded_topics_count", string_of_int(loadedTopicsCount)),
             |],
             "topics_partially_loaded_text",
           )
     )
     |> str}
  </div>;
};

module Selectable = {
  type t =
    | TopicCategory(TopicCategory.t)
    | Solution(bool)
    | Title(string);

  let label = t =>
    switch (t) {
    | TopicCategory(_category) => Some("Category")
    | Title(_) => Some("Topic Title")
    | Solution(_) => Some("Solution")
    };

  let value = t =>
    switch (t) {
    | TopicCategory(category) => TopicCategory.name(category)
    | Title(search) => search
    | Solution(solution) => solution ? "Solved" : "Unsolved"
    };

  let searchString = t =>
    switch (t) {
    | TopicCategory(category) => "category " ++ TopicCategory.name(category)
    | Title(search) => search
    | Solution(_solution) => "solution solved unsolved"
    };

  let color = t => {
    switch (t) {
    | TopicCategory(_category) => "orange"
    | Title(_search) => "gray"
    | Solution(_solution) => "green"
    };
  };
  let topicCategory = topicCategory => TopicCategory(topicCategory);
  let title = search => Title(search);
  let solution = on => Solution(on);
};

module Multiselect = MultiselectDropdown.Make(Selectable);

let unselected = (topicCategories, state) => {
  let unselectedCategories =
    topicCategories
    |> Js.Array.filter(category =>
         state.filter.topicCategory
         |> OptionUtils.mapWithDefault(
              selectedCategory =>
                category->TopicCategory.id
                != selectedCategory->TopicCategory.id,
              true,
            )
       )
    |> Array.map(Selectable.topicCategory);

  let trimmedFilterString = state.filterString |> String.trim;

  let title =
    trimmedFilterString == ""
      ? [||] : [|Selectable.title(trimmedFilterString)|];

  let hasSolution =
    switch (state.filter.solution) {
    | `Solved => [|Selectable.solution(false)|]
    | `Unsolved => [|Selectable.solution(true)|]
    | `Unselected => [|
        Selectable.solution(true),
        Selectable.solution(false),
      |]
    };

  unselectedCategories
  |> Js.Array.concat(title)
  |> Js.Array.concat(hasSolution);
};

let selected = state => {
  let selectedCategory =
    state.filter.topicCategory
    |> OptionUtils.mapWithDefault(
         selectedCategory => [|Selectable.topicCategory(selectedCategory)|],
         [||],
       );

  let selectedSearchString =
    state.filter.title
    |> OptionUtils.mapWithDefault(
         title => {[|Selectable.title(title)|]},
         [||],
       );

  let selectedSolutionFilter =
    switch (state.filter.solution) {
    | `Solved => [|Selectable.solution(true)|]
    | `Unsolved => [|Selectable.solution(false)|]
    | `Unselected => [||]
    };

  selectedCategory
  |> Js.Array.concat(selectedSearchString)
  |> Js.Array.concat(selectedSolutionFilter);
};

let onSelectFilter = (send, selectable) =>
  switch (selectable) {
  | Selectable.TopicCategory(topicCategory) =>
    send(SelectTopicCategory(topicCategory))
  | Title(title) => send(SetSearchString(title))
  | Solution(onOrOff) =>
    let filter = onOrOff ? `Solved : `Unsolved;
    send(SetSolutionFilter(filter));
  };

let onDeselectFilter = (send, selectable) =>
  switch (selectable) {
  | Selectable.TopicCategory(_topicCategory) => send(DeselectTopicCategory)
  | Title(_title) => send(UnsetSearchString)
  | Solution(_) => send(SetSolutionFilter(`Unselected))
  };

let filterPlaceholder = (state, topicCategories) => {
  switch (state.filter.topicCategory, state.filter.title) {
  | (None, None) =>
    ArrayUtils.isEmpty(topicCategories)
      ? t("filter_input_placeholder_default")
      : t("filter_input_placeholder_categories")
  | _ => ""
  };
};

let categoryDropdownSelected = topicCategory => {
  <div
    ariaLabel="Selected category filter"
    className="text-sm bg-white border border-gray-400 rounded py-1 md:py-2 px-3 focus:outline-none focus:bg-white focus:border-primary-300 cursor-pointer">
    {switch (topicCategory) {
     | Some(topicCategory) =>
       let (color, _) = TopicCategory.color(topicCategory);
       let style = ReactDOMRe.Style.make(~backgroundColor=color, ());

       <div className="inline-flex items-center">
         <div className="h-3 w-3 border" style />
         <span className="ml-2">
           {TopicCategory.name(topicCategory)->str}
         </span>
       </div>;
     | None => t("all_categories_button")->str
     }}
    <FaIcon classes="ml-4 fas fa-caret-down" />
  </div>;
};

let categoryDropdownContents =
    (availableTopicCategories, selectedTopicCategory, send) => {
  let selectableTopicCategories =
    Belt.Option.mapWithDefault(
      selectedTopicCategory, availableTopicCategories, topicCategory => {
      Js.Array.filter(
        availableTopicCategory =>
          TopicCategory.id(availableTopicCategory)
          != TopicCategory.id(topicCategory),
        availableTopicCategories,
      )
    });

  Js.Array.map(
    topicCategory => {
      let (color, _) = TopicCategory.color(topicCategory);
      let style = ReactDOMRe.Style.make(~backgroundColor=color, ());
      let categoryName = TopicCategory.name(topicCategory);

      <div
        ariaLabel={"Select category " ++ categoryName}
        className="pl-3 pr-4 py-2 font-normal flex items-center"
        onClick={_ => send(SelectTopicCategory(topicCategory))}>
        <div className="w-3 h-3 rounded" style />
        <span className="ml-1"> categoryName->str </span>
      </div>;
    },
    selectableTopicCategories,
  );
};

module Sortable = {
  type t = sortCriterion;

  let criterion = c =>
    switch (c) {
    | `CreatedAt => t("sort_criterion_posted_at")
    | `LastActivityAt => t("sort_criterion_last_activity")
    | `Views => t("sort_criterion_views")
    };

  let criterionType = _t => `Number;
};

module TopicsSorter = Sorter.Make(Sortable);

let topicsSorter = (state, send) => {
  <div ariaLabel="Change topics sorting" className="flex-shrink-0">
    <label className="block text-tiny font-semibold uppercase">
      {t("sort_criterion_input_label")->str}
    </label>
    <TopicsSorter
      criteria=[|`CreatedAt, `LastActivityAt, `Views|]
      selectedCriterion={state.sortCriterion}
      direction={state.sortDirection}
      onDirectionChange={sortDirection => {
        send(UpdateSortDirection(sortDirection))
      }}
      onCriterionChange={sortCriterion =>
        send(UpdateSortCriterion(sortCriterion))
      }
    />
  </div>;
};

[@react.component]
let make = (~communityId, ~target, ~topicCategories) => {
  let (state, send) =
    React.useReducerWithMapState(reducer, target, computeInitialState);

  React.useEffect3(
    () => {
      reloadTopics(communityId, state, send);
      None;
    },
    (state.filter, state.sortDirection, state.sortCriterion),
  );
  <div className="flex-1 flex flex-col">
    {switch (target) {
     | Some(target) =>
       <div className="max-w-3xl w-full mt-5 mx-auto px-3 md:px-0">
         <div
           className="flex py-4 px-4 md:px-6 w-full bg-yellow-100 border border-dashed border-yellow-400 rounded justify-between items-center">
           <p className="w-3/5 md:w-4/5 font-semibold text-sm">
             {"Target: " ++ Target.title(target) |> str}
           </p>
           <a
             className="no-underline bg-yellow-100 border border-yellow-400 px-3 py-2 hover:bg-yellow-200 rounded-lg cursor-pointer text-xs font-semibold"
             href={"/communities/" ++ communityId}>
             {"Clear Filter" |> str}
           </a>
         </div>
       </div>
     | None => React.null
     }}
    <div className="mt-5 flex flex-col flex-1 ">
      <div className="w-full sticky top-0 z-30 bg-gray-100 py-2">
        <div className="max-w-3xl w-full mx-auto relative px-3 md:px-6">
          <div className="pb-3 flex justify-between">
            {ReactUtils.nullIf(
               <Dropdown
                 selected={categoryDropdownSelected(
                   state.filter.topicCategory,
                 )}
                 contents={categoryDropdownContents(
                   topicCategories,
                   state.filter.topicCategory,
                   send,
                 )}
                 className=""
               />,
               ArrayUtils.isEmpty(topicCategories),
             )}
            <div
              className=""
              // Other controls go here.
            />
          </div>
          <div className="flex w-full items-start flex-wrap">
            <div className="flex-1 pr-2">
              <label className="block text-tiny font-semibold uppercase pl-px">
                {t("filter_input_label")->str}
              </label>
              <Multiselect
                id="filter"
                unselected={unselected(topicCategories, state)}
                selected={selected(state)}
                onSelect={onSelectFilter(send)}
                onDeselect={onDeselectFilter(send)}
                value={state.filterString}
                onChange={filterString =>
                  send(UpdateFilterString(filterString))
                }
                placeholder={filterPlaceholder(state, topicCategories)}
              />
            </div>
            {topicsSorter(state, send)}
          </div>
        </div>
      </div>
      <div className="max-w-3xl w-full mx-auto relative px-3 md:px-6 pb-4">
        <div id="topics" className="community-topic__list-container my-4">
          {switch (state.topics) {
           | Unloaded =>
             SkeletonLoading.multiple(
               ~count=10,
               ~element=SkeletonLoading.card(),
             )
           | PartiallyLoaded(topics, cursor) =>
             <div>
               <div className="shadow bg-white rounded-lg divide-y">
                 {topicsList(topicCategories, topics)}
               </div>
               <div className="text-center">
                 {topicsLoadedData(
                    state.totalTopicsCount,
                    Array.length(topics),
                  )}
               </div>
               {switch (state.loading) {
                | LoadingMore =>
                  SkeletonLoading.multiple(
                    ~count=3,
                    ~element=SkeletonLoading.card(),
                  )
                | NotLoading =>
                  <button
                    className="btn btn-primary-ghost cursor-pointer w-full mt-4"
                    onClick={_ => {
                      send(BeginLoadingMore);
                      getTopics(
                        send,
                        communityId,
                        Some(cursor),
                        state.filter,
                        state.sortCriterion,
                        state.sortDirection,
                      );
                    }}>
                    {t("button_load_more") |> str}
                  </button>
                | Reloading => React.null
                }}
             </div>
           | FullyLoaded(topics) =>
             <div>
               <div className="shadow bg-white rounded-lg divide-y">
                 {topicsList(topicCategories, topics)}
               </div>
               <div className="text-center">
                 {topicsLoadedData(
                    state.totalTopicsCount,
                    Array.length(topics),
                  )}
               </div>
             </div>
           }}
        </div>
      </div>
      {switch (state.topics) {
       | Unloaded => React.null

       | _ =>
         let loading =
           switch (state.loading) {
           | NotLoading => false
           | Reloading => true
           | LoadingMore => false
           };
         <LoadingSpinner loading />;
       }}
    </div>
  </div>;
};
