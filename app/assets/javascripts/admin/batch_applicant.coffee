setupSelect2ForBatchApplicantTagList = ->
  batchApplicantTagList = $('#batch_applicant_tag_list')

  if batchApplicantTagList.length
    currentFounderTags = batchApplicantTagList.data('tags')
    select2Data = $.map currentFounderTags, (tag) ->
      {
        id: tag,
        text: tag
      }

    batchApplicantTagList.select2(
      width: '80%',
      placeholder: 'Select some tags',
      tags: true,
      data: select2Data,
      createSearchChoice: (term, data) ->
        filteredData = $(data).filter ->
          this.text.localeCompare(term) == 0

        if filteredData.length == 0
          return {id: term, text: term}
    )

setupSelect2ForBatchApplicantColleges = ->
  collegeInput = $('#batch_applicant_college_id')

  if collegeInput.length
    collegeSearchUrl = collegeInput.data('searchUrl')

    collegeInput.select2
      width: '80%',
      minimumInputLength: 3,
      ajax:
        url: collegeSearchUrl,
        dataType: 'json',
        quietMillis: 500,
        data: (term, page) ->
          return {
            q: term
          }
        ,
        results: (data, page) ->
          return { results: data }
        cache: true

$(document).on 'page:change', ->
  $('#batch_applicant_batch_application_ids').select2(
    width: '80%',
    placeholder : 'Select applications'
  )

$(document).on 'page:change', setupSelect2ForBatchApplicantColleges
$(document).on 'page:change', setupSelect2ForBatchApplicantTagList
